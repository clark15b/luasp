CC	= gcc
CFLAGS	= -DLUA_MODULE `pkg-config lua5.1 --cflags` -shared -fPIC -fno-rtti -fno-exceptions -I/usr/include/mysql -L/usr/lib/mysql
LIBS	= -lmysqlclient

all:
	$(CC) $(CFLAGS) -o luamysql.so luamysql.cpp $(LIBS)
