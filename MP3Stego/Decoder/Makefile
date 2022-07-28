# MP3Stego
#
# Makefile for the decoder.
# Requires GNU make.
#
# Fabien A.P. Petitcolas
# Computer Laboratory, University of Cambridge
#
# $Header: /MP3Stego/MP3Stego Decoder/Makefile 5     11/02/99 11:28 Fapp2 $

PGM = decode # name of the program

CC = gcc

c_sources = \
	common.c \
	decode.c \
	huffman.c \
	ieeefloat.c \
	musicout.c \
	portableio.c

OBJS = $(c_sources:.c=.o)

NINT_SWITCH = #-DHAVE_NINT # if NINT is already implemented

AR = ar rc
RANLIB = ranlib
CC_SWITCHES = -g -O -DUNIX -DBS_FORMAT=BINARY -DNDEBUG
#	-pedantic -Wall -Wwrite-strings -Wpointer-arith \
#	-Wconversion -Wstrict-prototypes -Wmissing-prototypes

LIBS = libstego.a libz.a -lm

all: $(PGM)

$(PGM):	$(OBJS) Makefile libz.a libstego.a
	$(CC) -o $(PGM) $(OBJS) $(LIBS)

%.o: %.c  Makefile
	$(CC) $(CC_SWITCHES) -c $< -o $@

libz.a:
	cd ../../gzlib/;make libz.a;cd ../MP3Stego/decoder/
	ln -sf ../../gzlib/libz.a .

libstego.a:
	cd ../../stegolib/;make libstego.a;cd ../MP3Stego/decoder/
	ln -sf ../../stegolib/libstego.a .

clean:
	rm -f $(OBJS)
	cd ../../stegolib/;make clean
	cd ../../gzlib/;make clean


