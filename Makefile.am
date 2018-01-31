AM_CXXFLAGS = -g -Wall @libusb_CFLAGS@

bin_PROGRAMS = isd mem

isd_SOURCES = isd.cpp piecedev.cpp debug.cpp piecefat.cpp
isd_LDADD = @libusb_LIBS@

mem_SOURCES = mem.cpp piecedev.cpp debug.cpp
mem_LDADD = @libusb_LIBS@
