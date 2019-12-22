# 8-bit Xor Filter

This is a C99 implementation of an 8-bit xor filter. It has a fixed
error rate of 1/256 (~0.39%) and uses ~9.84 bits of storage per element.
See [Xor Filters: Faster and Smaller Than Bloom Filters][ref].

## Memory Usage

By default only up to 2^32 elements are supported, which will require
123GB of memory to process. Much larger sets are supported by using
`-DXF8_64BIT` at compile time, at the cost of roughly doubling memory
usage. For example, that same set of 2^32 elements will then require a
total of 211GB of memory to process.

Despite the library's name, the error rate can be reduced to 1/65,536
(~0.0015%) by using `-DXF8_BITS=16` at compile time. This doubles the
size of the filter to ~19.68 bits per element.

## Example

The example program (`tests/example.c`) is a probabilistic spell checker:

```
$ make
$ tests/example </usr/share/dict/words >spelling.db
$ printf 'hello\nfoobarbaz' | tests/example spelling.db
Y hello
N foobarbaz
```

[ref]: https://arxiv.org/abs/1912.08258
