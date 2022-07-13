libname=libtpool.so
OBJECTS=tpool.c
CFLAGS=-g -Wall
LIBFLAGS=-shared -fPIC
LDLIBS=-lpthread
CC=clang

all: $(libname) example

$(libname): $(OBJECTS)
	$(CC) -o $@ $(CFLAGS) $(LIBFLAGS) $< $(LDLIBS)

example: example.c $(OBJECTS)
	$(CC) -o $@ $(CFLAGS) -DTPOOL_VERBOSE=1 $^ $(LDLIBS)

clean:
	rm -rf example libtpool.so
