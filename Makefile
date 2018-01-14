CXXFLAGS=`libusb-config --cflags` -g -Wall -DNDEUBG
LDFLAGS=`libusb-config --libs` -g

ifeq ($(wildcard .depend),.depend)
all: isd
include .depend
else
all: depend remake
remake:
	make all
endif

OBJS=isd.o piecedev.o debug.o piecefat.o
MEM_OBJS=mem.o piecedev.o debug.o

isd: $(OBJS)
	g++ $(LDFLAGS) -o $@ $(OBJS)
mem: $(MEM_OBJS)
	g++ $(LDFLAGS) -o $@ $(MEM_OBJS)

clean: 
	rm -f *.o .depend isd mem

.PHONY: depend
depend:
	gcc -MM *.cpp > .depend
