CC      = cc -std=c99
CFLAGS  = -O3 -Wall -Wextra
LDFLAGS =
LDLIBS  =

example: example.c xf8.c xf8.h
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ example.c xf8.c $(LDLIBS)

clean:
	rm -f example
