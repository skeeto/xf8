/* This is free and unencumbered software released into the public domain. */
#include <stdint.h>
#include <stdlib.h>
#include "xf8.h"

struct xf8 *
xf8_create(size_t count)
{
    size_t len = 123*count/100 + 32; // FIXME: overflow check
    len += (3 - len % 3) % 3; // round up to divisible by 3
    struct xf8 *xf = malloc(sizeof(struct xf8) + len);
    if (xf) {
        xf->len = len;
        xf->seed = 0;
    }
    return xf;
}

static uint64_t
xf8_hash(uint64_t x)
{
    x ^= x >> 32;
    x *= UINT64_C(0xc6629b183fbdc9a7);
    x ^= x >> 32;
    x *= UINT64_C(0xc029435c0845c0b3);
    x ^= x >> 32;
    return x;
}

/* Compute the 3-tuple of indices for KEY. */
static void
xf8_index(const struct xf8 *xf, size_t c[3], uint64_t key)
{
    size_t len = xf->len / 3;
    // TODO: avoid division by unknown denominator
    c[0] = xf8_hash(key + xf->seed*3 + 0)%len + len*0;
    c[1] = xf8_hash(key + xf->seed*3 + 1)%len + len*1;
    c[2] = xf8_hash(key + xf->seed*3 + 2)%len + len*2;
}

void
xf8_populate(struct xf8 *xf, uint64_t *keys, size_t count)
{
    /* Sets are represented using linked lists with all nodes allocated
     * contiguously up front. There are no pointers, just indices into
     * the array of linked list nodes, so each third of the set array
     * needs its own allocation of linked list nodes. The special index
     * value of -1 is like a NULL pointer.
     *
     * The queue and stack use the same storage: As the queue is
     * consumed, the stack overwrites it. The worst case allocation is
     * used for the queue so it never needs to be reallocated.
     *
     * Since all values are size_t indices, just allocate everything up
     * front as a giant size_t buffer, then dice it up.
     */
    // FIXME: overflow check
    size_t *buf = malloc((2*xf->len + 3*count)*sizeof(*buf));
    size_t *sets = buf;
    size_t *queue = buf + xf->len;
    size_t *stack = queue;
    size_t *nodes[3] = {
        buf + 2*xf->len + count*0,
        buf + 2*xf->len + count*1,
        buf + 2*xf->len + count*2
    };
    xf->seed = 0;

    for (;;) {
        /* Initialize all sets to empty. */
        for (size_t i = 0; i < xf->len; i++) {
            sets[i] = -1;
        }

        /* Fills sets with the keys. */
        for (size_t i = 0; i < count; i++) {
            size_t c[3];
            xf8_index(xf, c, keys[i]);
            for (int j = 0; j < 3; j++) {
                size_t *p = sets + c[j];
                while (*p != (size_t)-1) {
                    p = nodes[j] + *p;
                }
                *p = i;
                nodes[j][i] = -1;
            }
        }

        /* Queue all sets with exactly one element. */
        size_t head = 0;
        size_t tail = 0;
        for (size_t i = 0; i < xf->len; i++) {
            int j = i / (xf->len / 3);
            if (sets[i] != (size_t)-1 && nodes[j][sets[i]] == (size_t)-1) {
                queue[head++] = i;
            }
        }

        /* Process the queue until empty. */
        size_t top = 0;
        while (head != tail) {
            size_t i = queue[tail++];
            if (sets[i] != (size_t)-1) {
                size_t k = sets[i];
                size_t c[3];
                xf8_index(xf, c, keys[k]);
                for (int j = 0; j < 3; j++) {
                    size_t *p = sets + c[j];
                    while (*p != k) {
                        p = nodes[j] + *p;
                    }
                    *p = nodes[j][k];
                    size_t h = sets[c[j]];
                    if (h != (size_t)-1 && nodes[j][h] == (size_t)-1) {
                        queue[head++] = c[j];
                    }
                }

                /* Push this key index onto the stack. The first set of
                 * linked list nodes is re-purposed to track the set index
                 * that belongs to this key.
                 */
                stack[top++] = k;
                nodes[0][k] = i;
            }
        }

        if (top == count) {
            /* Success! Fill out the XOR filter with the results. */
            for (size_t i = 0; i < xf->len; i++) {
                xf->slots[i] = 0;
            }
            while (top) {
                size_t k = stack[--top];
                size_t i = nodes[0][k];
                size_t c[3];
                xf8_index(xf, c, keys[k]);
                unsigned char *b = xf->slots;
                xf->slots[i] = b[c[0]] ^ b[c[1]] ^ b[c[2]] ^ keys[k];
            }
            break;
        }

        /* Failure, increment seed and try again with a new set of hash
         * functions. This is unimaginably unlikely to ever overflow, so
         * don't sweat it.
         */
        xf->seed++;
    }

    free(buf);
}

int
xf8_member(const struct xf8 *xf, uint64_t key)
{
    size_t c[3];
    xf8_index(xf, c, key);
    const unsigned char *b = xf->slots;
    return (unsigned char)key == (b[c[0]] ^ b[c[1]] ^ b[c[2]]);
}
