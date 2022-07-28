# MP3Stego
#
# Simplified makefile for the Z library.
# Requires GNU make.
#
# Fabien A.P. Petitcolas
# Computer Laboratory, University of Cambridge

CC = gcc

c_sources = \
	adler32.c \
	compress.c \
	crc32.c \
	gzio.c \
	uncompr.c \
	deflate.c \
	trees.c \
	zutil.c \
	inflate.c \
	infblock.c \
	inftrees.c \
	infcodes.c \
	infutil.c \
	inffast.c

OBJS = $(c_sources:.c=.o)

AR = ar rc
RANLIB = ranlib
CC_SWITCHES = -O3 -DHAVE_UNISTD_H -DUSE_MMAP  -Wall -pedantic -Wall \
	-Wwrite-strings -Wpointer-arith -Wconversion \
	-Wstrict-prototypes -Wmissing-prototypes

all: libz.a

libz.a: $(OBJS) Makefile
	$(AR) $@ $(OBJS) $(OBJA)
	-@ ($(RANLIB) $@ || true) >/dev/null 2>&1

%.o: %.c  Makefile
	$(CC) $(CC_SWITCHES) -c $< -o $@
clean:
	rm -f $(OBJS)

# DO NOT DELETE THIS LINE -- make depend depends on it.

adler32.o: zlib.h zconf.h
compress.o: zlib.h zconf.h
crc32.o: zlib.h zconf.h
deflate.o: deflate.h zutil.h zlib.h zconf.h
example.o: zlib.h zconf.h
gzio.o: zutil.h zlib.h zconf.h
infblock.o: infblock.h inftrees.h infcodes.h infutil.h zutil.h zlib.h zconf.h
infcodes.o: zutil.h zlib.h zconf.h
infcodes.o: inftrees.h infblock.h infcodes.h infutil.h inffast.h
inffast.o: zutil.h zlib.h zconf.h inftrees.h
inffast.o: infblock.h infcodes.h infutil.h inffast.h
inflate.o: zutil.h zlib.h zconf.h infblock.h
inftrees.o: zutil.h zlib.h zconf.h inftrees.h
infutil.o: zutil.h zlib.h zconf.h infblock.h inftrees.h infcodes.h infutil.h
minigzip.o:  zlib.h zconf.h 
trees.o: deflate.h zutil.h zlib.h zconf.h trees.h
uncompr.o: zlib.h zconf.h
zutil.o: zutil.h zlib.h zconf.h  
