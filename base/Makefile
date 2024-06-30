include Makefile.conf
DIR = base/

INCLUDE = ../include
CXXFLAGS += -I/usr/include/libxml2

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
	termlog		\
	tki		\
	transport	\
	xbase64		\
	xchange

BASE_CPP_OBJS =		\
	xml2data

OBJS = $(addsuffix .o, $(BASE_OBJS) $(BASE_CPP_OBJS))

all:		depend $(OBJS)

depend:
		@$(CC) $(CFLAGS) -MM $(addsuffix .c, $(BASE_OBJS)) > .depend
		@$(CC) $(CXXFLAGS) -MM $(addsuffix .cpp, $(BASE_CPP_OBJS)) >> .depend

clean:
		@rm -f $(BIN_GARBAGE)

ifneq ($(wildcard .depend),)
include .depend
endif
