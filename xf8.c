/* This is free and unencumbered software released into the public domain. */
#include <stdint.h>
#include <stdlib.h>
#include "xf8.h"

/* Adjust this typedef as needed to control the maximum allowed number
 * of keys at the cost of higher memory usage. As a 32-bit integer, up
 * to 2^32 keys are supported, which will require 91GB of memory in
 * order to build the filter. Using a uint64_t allows for a larger
 * number of keys, but exactly doubles memory usage.
 */
typedef uint32_t fx8uint;
#define FX8NULL ((fx8uint)-1)

struct xf8 *
xf8_create(size_t count)
{
    struct xf8 *xf = 0;
    unsigned long long len = 123ULL*count/100 + 32;
    len += (3 - len % 3) % 3; // round up to divisible by 3
    if (count < -1ULL/123 && len < (size_t)-1) {
        xf = malloc(sizeof(struct xf8) + len);
        if (xf) {
            xf->len = len;
        }
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
xf8_index(const struct xf8 *xf, fx8uint c[3], uint64_t key)
{
    size_t len = xf->len / 3;
    // TODO: avoid division by unknown denominator
    c[0] = xf8_hash(key + xf->seed*3 + 0)%len + len*0;
    c[1] = xf8_hash(key + xf->seed*3 + 1)%len + len*1;
    c[2] = xf8_hash(key + xf->seed*3 + 2)%len + len*2;
}

int
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
     * Since all values are fx8uint indices, just allocate everything up
     * front as a giant fx8uint buffer, then dice it up. (There's some
     * potential here for allowing the caller to make this allocation.)
     */
    if (xf->len > (size_t)-1/(5*sizeof(fx8uint))) {
        return 0; // overflow
    }
    fx8uint *buf = malloc((2*xf->len + 3*count)*sizeof(*buf));
    if (!buf) {
        return 0;
    }
    fx8uint *sets = buf;
    fx8uint *queue = buf + xf->len;
    fx8uint *stack = queue;
    fx8uint *nodes[3] = {
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
            fx8uint c[3];
            xf8_index(xf, c, keys[i]);
            for (int j = 0; j < 3; j++) {
                nodes[j][i] = sets[c[j]];
                sets[c[j]] = i;
            }
        }

        /* Queue all sets with exactly one element. */
        fx8uint head = 0;
        fx8uint tail = 0;
        for (size_t i = 0; i < xf->len; i++) {
            int j = i / (xf->len / 3);
            if (sets[i] != FX8NULL && nodes[j][sets[i]] == FX8NULL) {
                queue[head++] = i;
            }
        }

        /* Process the queue until empty. */
        fx8uint top = 0;
        while (head != tail) {
            fx8uint i = queue[tail++];
            if (sets[i] != FX8NULL) {
                fx8uint k = sets[i];
                fx8uint c[3];
                xf8_index(xf, c, keys[k]);
                for (int j = 0; j < 3; j++) {
                    fx8uint *p = sets + c[j];
                    while (*p != k) {
                        p = nodes[j] + *p;
                    }
                    *p = nodes[j][k];
                    fx8uint h = sets[c[j]];
                    if (h != FX8NULL && nodes[j][h] == FX8NULL) {
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
                fx8uint k = stack[--top];
                fx8uint i = nodes[0][k];
                fx8uint c[3];
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
    return 1;
}

int
xf8_member(const struct xf8 *xf, uint64_t key)
{
    fx8uint c[3];
    xf8_index(xf, c, key);
    const unsigned char *b = xf->slots;
    return (unsigned char)key == (b[c[0]] ^ b[c[1]] ^ b[c[2]]);
}
