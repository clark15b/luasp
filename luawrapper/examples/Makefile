CC	= g++
CFLAGS	= -I.. `pkg-config --cflags lua5.1`
LIBS	= `pkg-config --libs lua5.1`
OBJS	= ../luawrapper.o
EXAMPLS	= exampl1 exampl2 exampl3

all: $(OBJS) $(EXAMPLS)
	rm -f $(OBJS)

clean:
	rm -f $(EXAMPLS)

.cpp:
	$(CC) $(CFLAGS) -o $@ $< $(OBJS) $(LIBS)

.cpp.o:
	$(CC) $(CFLAGS) -c -o $@ $<
