# MP3Stego
#
# Makefile for the encoder.
# Requires GNU make.
#
# Fabien A.P. Petitcolas
# Computer Laboratory, University of Cambridge
#
# $Header: /MP3Stego/MP3Stego Encoder/Makefile 5     11/02/99 11:29 Fapp2 $

PGM = encode # program name

CC = gcc

c_sources = \
	bitstream.c \
	formatBitstream.c \
	ieeefloat.c \
	l3loop.c \
	l3psy.c \
	layer3.c \
	portableio.c \
	tables.c \
	fft.c \
	huffman.c \
	l3bitstream.c \
	l3mdct.c \
	l3subband.c \
	main.c \
	reservoir.c \
	wave.c

OBJS = $(c_sources:.c=.o)

NINT_SWITCH = #-DHAVE_NINT # if NINT is already implemented

AR = ar rc
RANLIB = ranlib
CC_SWITCHES = -g -O -DUNIX -DBS_FORMAT=BINARY -DNDEBUG $(NINT_SWITCH)
#	-pedantic -Wall -Wwrite-strings -Wpointer-arith \
#	-Wconversion -Wstrict-prototypes -Wmissing-prototypes

LIBS = libstego.a libz.a -lm

all: $(PGM)

$(PGM):	$(OBJS) Makefile libz.a libstego.a
	$(CC) -o $(PGM) $(OBJS) $(LIBS)

%.o: %.c  Makefile
	$(CC) $(CC_SWITCHES) -c $< -o $@

libz.a:
	cd ../../gzlib/;make libz.a;cd ../MP3Stego/encoder/
	ln -sf ../../gzlib/libz.a .

libstego.a:
	cd ../../stegolib/;make libstego.a;cd ../MP3Stego/encoder/
	ln -sf ../../stegolib/libstego.a .

clean:
	rm -f $(OBJS)
	cd ../../stegolib/;make clean
	cd ../../gzlib/;make clean




