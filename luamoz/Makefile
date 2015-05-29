CC	= gcc
CFLAGS	= `pkg-config lua5.1 --cflags` -fPIC -I.. -L..
OBJS	= luamoz.o md5c.o

#CFLAGS += -DIGNORE_EXP_DATE

all: $(OBJS)
	$(CC) $(CFLAGS) -shared -o luamoz.so $(OBJS)
	rm -rf $(OBJS)

.c.o:
	$(CC) -c $(CFLAGS) -o $@ $<
        
.cpp.o:
	$(CC) -c $(CFLAGS) -fno-rtti -fno-exceptions -o $@ $<
                