/* Probabilistic spell checker example using a xor filter */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>
#include <string.h>
#include "xf8.h"

static uint64_t
hash(const void *buf, size_t len, uint64_t key)
{
    const unsigned char *p = buf;
    uint64_t h = key;
    for (size_t i = 0; i < len; i++) {
        h ^= p[i];
        h *= UINT64_C(0x25b751109e05be63);
    }
    h ^= h >> 32;
    h *= UINT64_C(0x2330e1453ed4b9b9);
    return h;
}

int
main(int argc, char *argv[])
{
    uint64_t hashkey = 0x648aaecaca11a629;

    if (argc == 1) {
        char buf[4096];
        long count = 0;
        static uint64_t keys[1L<<20];
        while (fgets(buf, sizeof(buf), stdin) && count < 1L<<20) {
            size_t len = strcspn(buf, "\r\n");
            keys[count++] = hash(buf, len, hashkey);
        }

        struct xf8 *xf = xf8_create(count);
        xf8_populate(xf, keys, count);
        fputc(count >>  0 & 0xff, stdout);
        fputc(count >>  8 & 0xff, stdout);
        fputc(count >> 16 & 0xff, stdout);
        fputc(xf->seed, stdout);
        fwrite(xf->slots, xf->len, 1, stdout);
        free(xf);

        if (fflush(stdout)) {  // note: is not a 100% sufficient check
            fprintf(stderr, "fatal: %s, <stdout>\n", strerror(errno));
            exit(EXIT_FAILURE);
        }

    } else {
        FILE *f = fopen(argv[1], "rb");
        if (!f) {
            fprintf(stderr, "fatal: %s, %s\n", strerror(errno), argv[1]);
            exit(EXIT_FAILURE);
        }

        unsigned char header[4];
        if (!fread(header, sizeof(header), 1, f)) {
            fprintf(stderr, "fatal: cannot read %s\n", argv[1]);
            exit(EXIT_FAILURE);
        }
        unsigned long count = (unsigned long)header[0] <<  0 |
                              (unsigned long)header[1] <<  8 |
                              (unsigned long)header[2] << 16;
        struct xf8 *xf = xf8_create(count);
        xf->seed = header[3];
        if (!fread(xf->slots, xf->len, 1, f)) {
            fprintf(stderr, "fatal: cannot read %s\n", argv[1]);
            exit(EXIT_FAILURE);
        }
        fclose(f);

        char buf[4096];
        while (fgets(buf, sizeof(buf), stdin)) {
            size_t len = strcspn(buf, "\r\n");
            uint64_t key = hash(buf, len, hashkey);
            putchar(xf8_member(xf, key) ? 'Y' : 'N');
            putchar(' ');
            fwrite(buf, len, 1, stdout);
            putchar('\n');
        }

        free(xf);
    }
}
