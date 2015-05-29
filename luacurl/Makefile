CC	= gcc
CFLAGS	= -DLUA_MODULE `pkg-config libcurl lua5.1 --cflags` -shared -fPIC -fno-rtti -fno-exceptions
LIBS	= `pkg-config libcurl --libs`

all:
	$(CC) $(CFLAGS) -o luacurl.so luacurl.cpp $(LIBS)
