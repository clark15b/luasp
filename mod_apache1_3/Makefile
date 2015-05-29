CC	= gcc
APXS	= /usr/local/apache/bin/apxs
APCTL	= /usr/local/apache/bin/apachectl

CFLAGS	= -I../ `pkg-config lua5.1 libcurl uuid --cflags` -fno-rtti -fno-exceptions -DAPACHE13 -I`$(APXS) -q INCLUDEDIR` \
 -I../luamysql -I../luacurl -I/usr/include/mysql -L/usr/lib/mysql -I../luajson -I../luamemcache -DVERSION=\"luasp/`date +%Y%m%d%H%M%S`\"
LIBS    = `pkg-config lua5.1 libcurl uuid --libs` -lmysqlclient
SRC	= mod_luasp.cpp ../llsplib.cpp ../llspaux.cpp ../luamysql/luamysql.cpp ../luacurl/luacurl.cpp ../luajson/luajson.cpp ../luajson/luajson_parser.cpp ../luamemcache/luamcache.cpp

all: apache
	strip mod_luasp.so

apache:
	$(CC) $(CFLAGS) -shared -fPIC -o mod_luasp.so $(SRC) $(LIBS)

apache_eapi:
	$(CC) $(CFLAGS) -shared -fPIC -DEAPI -o mod_luasp.so $(SRC) $(LIBS)

install:
	$(APCTL) stop
	sleep 1
	cp mod_luasp.so `$(APXS) -q LIBEXECDIR`
	$(APCTL) start
