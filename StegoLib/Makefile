# MP3Stego
#
# Makefile for the stego library.
# Requires GNU make.
#
# Fabien A.P. Petitcolas
# Computer Laboratory, University of Cambridge
#
# $Header: /StegoLib/Makefile 4     11/02/99 11:29 Fapp2 $

CC = gcc

c_sources = \
	des_enc.c \
	ede_enc.c \
	set_key.c \
	sha.c \
	stego.c \
	tools.c \
	error.c

OBJS = $(c_sources:.c=.o)

AR = ar rc
RANLIB = ranlib
CC_SWITCHES = -g -O -DUNIX -DNDEBUG -Wall -pedantic

all: libstego.a

libstego.a: $(OBJS) Makefile
	$(AR) $@ $(OBJS)
	-@ ($(RANLIB) $@ || true) >/dev/null 2>&1

%.o: %.c  Makefile
	$(CC) $(CC_SWITCHES) -c $< -o $@

clean:
	rm -f $(OBJS)


