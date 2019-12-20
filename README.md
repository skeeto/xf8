# 8-bit Xor Filter

This is a C99 implementation of an 8-bit xor filter. It has a fixed
error rate of 1/256 (~0.39%) and uses ~9.84 bits of storage per element.
See [Xor Filters: Faster and Smaller Than Bloom Filters][ref].

The example program is a probabilistic spell checker:

```
$ make
$ ./example </usr/share/dict/words >spelling.db
$ printf 'hello\nfoobarbaz' | ./example spelling.db
  Y hello
  N foobarbaz
```

[ref]: https://arxiv.org/abs/1912.08258
