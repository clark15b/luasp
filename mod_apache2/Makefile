CC	= gcc
APXS	= apxs2
APRCONF = apr-config
APCTL   = apache2ctl

CFLAGS	= -I../ `pkg-config lua5.1 libcurl uuid --cflags` -fno-rtti -fno-exceptions -I`$(APXS) -q INCLUDEDIR` `$(APRCONF) --includes` \
 -I../luamysql -I../luacurl -I/usr/include/mysql -L/usr/lib/mysql -I../luajson -I../luamemcache -DVERSION=\"luasp/`date +%Y%m%d%H%M%S`\"
LIBS    = `pkg-config lua5.1 libcurl uuid --libs` -lmysqlclient
SRC	= mod_luasp.cpp ../llsplib.cpp ../llspaux.cpp ../luamysql/luamysql.cpp ../luacurl/luacurl.cpp ../luajson/luajson.cpp ../luajson/luajson_parser.cpp ../luamemcache/luamcache.cpp

all: apache

apache:
	$(CC) -DTHREAD_SAFE $(CFLAGS) -shared -fPIC -o mod_luasp.so $(SRC) $(LIBS)
	strip mod_luasp.so

apache-prefork:
	$(CC) $(CFLAGS) -shared -fPIC -o mod_luasp.so $(SRC) $(LIBS)
	strip mod_luasp.so


install:
	$(APCTL) stop
	sleep 1
	cp mod_luasp.so `$(APXS) -q LIBEXECDIR`
	$(APCTL) start