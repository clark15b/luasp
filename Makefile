CC	= gcc
CFLAGS	= `pkg-config lua5.1 --cflags` -fno-rtti -fno-exceptions -DTHREAD_SAFE
LIBS    = `pkg-config lua5.1 --libs`


all: lua_module

lua_module:
	$(CC) $(CFLAGS) -shared -fPIC -o lualsp.so llsplib.cpp llspaux.cpp

demo:
	$(CC) $(CFLAGS) -o test test.cpp llsplib.cpp llspaux.cpp $(LIBS)
	./test test.lsp
