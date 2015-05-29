CC	= gcc
CFLAGS	= -DLUA_MODULE `pkg-config lua5.1 --cflags` -shared -fPIC -fno-rtti -fno-exceptions
LIBS	= 

all:
	$(CC) $(CFLAGS) -o luamcache.so luamcache.cpp $(LIBS)
	lua test.lua