/* 8-bit Xor filter
 * Filters have a fixed false positive rate of 1/256 (~0.39%).
 * Ref: https://arxiv.org/abs/1912.08258
 *
 * This is free and unencumbered software released into the public domain.
 */
#ifndef XF8_H
#define XF8_H

#include <stddef.h>
#include <stdint.h>

#if !defined(XF8_BITS) || XF8_BITS == 8
typedef uint8_t xf8slot;
#elif XF8_BITS == 16
typedef uint16_t xf8slot;
#endif

struct xf8 {
    size_t len;
    int seed;
    xf8slot slots[];
};

/**
 * Allocate a new XOR filter for COUNT elements, returns NULL on OOM.
 * This is a simple, flat allocation that is freed using free().
 */
struct xf8 *xf8_create(size_t count);

/**
 * Set the filter for the given KEYS, returns 0 on OOM.
 * There MUST NOT be repeated keys.
 */
int xf8_populate(struct xf8 *, uint64_t *keys, size_t count);

/**
 * Return non-zero if KEY is probably a member of the XOR filter.
 */
int xf8_member(const struct xf8 *, uint64_t key);

#endif
