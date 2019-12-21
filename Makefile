CC      = cc -std=c99
CFLAGS  = -O3 -Wall -Wextra
LDFLAGS =
LDLIBS  =

all: tests/example tests/test

tests/example: tests/example.c xf8.c xf8.h
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ tests/example.c xf8.c $(LDLIBS)

tests/test: tests/test.c xf8.c xf8.h
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ tests/test.c xf8.c $(LDLIBS)

check: tests/test
	tests/test

clean:
	rm -f tests/example tests/test
