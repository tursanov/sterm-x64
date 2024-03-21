include Makefile.conf
DIR = base/

INCLUDE = ../include
CFLAGS += -I/usr/include/libxml2
BASE_OBJS =		\
	base64		\
	cfg		\
	config		\
	devinfo		\
	ds1990a		\
	express		\
	gd		\
	genfunc		\
	hash		\
	iplir		\
	kbd		\
	keys		\
	licsig		\
	list		\
	md5		\
	numbers		\
	rndtbl		\
	serial		\
	serialize	\
	sterm		\
	tki		\
	transport	\
	xchange

OBJS = $(addsuffix .o, $(BASE_OBJS))

all:		depend $(OBJS)

depend:
		@$(CC) $(CFLAGS) -MM $(OBJS:.o=.c) > .depend

clean:
		@rm -f $(BIN_GARBAGE)

ifneq ($(wildcard .depend),)
include .depend
endif
