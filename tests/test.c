#include "../xf8.h"
#include <stdio.h>
#include <stdlib.h>

static uint64_t
hash(uint64_t x)
{
    x ^= x >> 32;
    x *= 0x78a1303cf234a045;
    x ^= x >> 32;
    x *= 0x098a71082462c3d9;
    x ^= x >> 32;
    return x;
}

static char *
test(uint64_t seed, long n)
{
    struct xf8 *xf = xf8_create(n);
    if (!xf) return 0;

    uint64_t *keys = malloc(sizeof(keys[0])*n);
    if (!keys) {
        free(xf);
        return "OOM";
    }
    for (long i = 0; i < n; i++) {
        keys[i] = hash(seed + i);
    }

    if (!xf8_populate(xf, keys, n)) {
        free(keys);
        free(xf);
        return "OOM";
    }
    free(keys);

    for (long i = 0; i < n; i++) {
        uint64_t key = hash(seed + i);
        if (!xf8_member(xf, key)) {
            free(xf);
            return "xf8_member";
        }
    }

    long hit = 0;
    for (long i = 1; i >= -n; i--) {
        uint64_t key = hash(seed + i);
        hit += xf8_member(xf, key);
    }

    double rate = hit / (double)n;
    if (rate > 1.5/(sizeof(xf8slot)*8)) {
        static char tmp[256];
        sprintf(tmp, "too many false positives: %.17g%%\n", rate*100);
        return tmp;
    }

    free(xf);
    return 0;
}

int
main(void)
{
    int fails = 0;

#ifdef _WIN32
    /* Best effort enable ANSI escape processing. */
    void *GetStdHandle(unsigned);
    int GetConsoleMode(void *, unsigned *);
    int SetConsoleMode(void *, unsigned);
    void *handle;
    unsigned mode;
    handle = GetStdHandle(-11); /* STD_OUTPUT_HANDLE */
    if (GetConsoleMode(handle, &mode)) {
        mode |= 0x0004; /* ENABLE_VIRTUAL_TERMINAL_PROCESSING */
        SetConsoleMode(handle, mode); /* ignore errors */
    }
#endif

    for (int i = 13; i <= 22; i++) {
        int level = 0;
        for (int j = 0; j < 8; j++) {
            uint64_t seed = hash(i) + hash(-j - 1);
            const char *result = test(seed, 1L<<i);
            if (result) {
                printf("\x1b[31;1mFAIL\x1b[0m: 1L<<%d, %s\n", i, result);
                fails++;
                level++;
            }
        }
        if (!level) {
            printf("\x1b[32;1mPASS\x1b[0m: 1L<<%d\n", i);
        }
    }

    return fails != 0;
}
