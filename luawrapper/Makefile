CC	= g++
CFLAGS	= `pkg-config --cflags --libs lua5.1`

all: test

test:
	$(CC) $(CFLAGS) -o test test.cpp luawrapper.cpp

clean:
	rm -f test