CC	= gcc
CFLAGS	= -DLUA_MODULE `pkg-config lua5.1 --cflags` -shared -fPIC -fno-rtti -fno-exceptions
LIBS	= 

all:
	$(CC) $(CFLAGS) -o luajson.so luajson.cpp luajson_parser.cpp $(LIBS)
	lua test.lua
