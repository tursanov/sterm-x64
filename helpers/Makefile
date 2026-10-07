include Makefile.conf
DIR = helpers/

INCLUDE = ../include
CFLAGS += -D__STANDALONE__

BINARIES =		\
	chkrmlic	\
	mkbind		\
	mkbnklic	\
	mklog		\
	mw

BASE_OBJS =		\
	base64		\
	genfunc		\
	md5		\
	rndtbl

HELPERS_OBJS =		\
	chkrmlic	\
	ds1990a		\
	licsig		\
	mkbind		\
	mkbnklic	\
	mklic		\
	mklog		\
	mw		\
	tki

DIR_BASE = ../base/

OBJS =			\
	$(addprefix $(DIR_BASE), $(addsuffix .o, $(BASE_OBJS))) \
	$(addsuffix .o, $(HELPERS_OBJS))

BIN_GARBAGE += $(BINARIES)

all:		depend $(BINARIES)

depend:
		@$(CC) $(CFLAGS) -MM $(OBJS:.o=.c) > .depend

## chkrmlic
CHKRMLIC_BASE_OBJS =	\
	base64		\
	md5		\
	rndtbl

CHKRMLIC_HELPERS_OBJS =	\
	chkrmlic	\
	licsig		\
	tki

CHKRMLIC_OBJS =		\
	$(addprefix $(DIR_BASE), $(addsuffix .o, $(CHKRMLIC_BASE_OBJS))) \
	$(addsuffix .o, $(CHKRMLIC_HELPERS_OBJS))

chkrmlic:	$(CHKRMLIC_OBJS)
		$(LD_BANNER)
		@$(CC) $(CFLAGS) -o $@ $^

## mkbind
MKBIND_BASE_OBJS =	\
	base64		\
	md5

MKBIND_HELPERS_OBJS =	\
	licsig		\
	mkbind		\
	tki

MKBIND_OBJS =		\
	$(addprefix $(DIR_BASE), $(addsuffix .o, $(MKBIND_BASE_OBJS))) \
	$(addsuffix .o, $(MKBIND_HELPERS_OBJS))

mkbind:		$(MKBIND_OBJS)
		$(LD_BANNER)
		@$(CC) $(CFLAGS) -o $@ $^

## mkbnklic
MKBNKLIC_BASE_OBJS =	\
	base64		\
	md5

MKBNKLIC_HELPERS_OBJS =	\
	licsig		\
	mkbnklic	\
	mklic		\
	tki

MKBNKLIC_OBJS =	\
	$(addprefix $(DIR_BASE), $(addsuffix .o, $(MKBNKLIC_BASE_OBJS))) \
	$(addsuffix .o, $(MKBNKLIC_HELPERS_OBJS))

mkbnklic:	$(MKBNKLIC_OBJS)
		$(LD_BANNER)
		@$(CC) $(CFLAGS) -o $@ $^

## mklog
MKLOG_BASE_OBJS =	\
	genfunc

MKLOG_HELPERS_OBJS =	\
	mklog

MKLOG_OBJS =		\
	$(addprefix $(DIR_BASE), $(addsuffix .o, $(MKLOG_BASE_OBJS))) \
	$(addsuffix .o, $(MKLOG_HELPERS_OBJS))

mklog:		$(MKLOG_OBJS)
		$(LD_BANNER)
		@$(CC) $(CFLAGS) -o $@ $^

## mw
MW_BASE_OBJS =		\
	base64		\
	genfunc		\
	md5

MW_HELPERS_OBJS =	\
	ds1990a		\
	licsig		\
	mw		\
	tki

MW_OBJS =		\
	$(addprefix $(DIR_BASE), $(addsuffix .o, $(MW_BASE_OBJS))) \
	$(addsuffix .o, $(MW_HELPERS_OBJS))

mw:		$(MW_OBJS)
		$(LD_BANNER)
		@$(CC) $(CFLAGS) -o $@ $^ -static

clean:
		@rm -f $(BIN_GARBAGE)

ifneq ($(wildcard .depend),)
include .depend
endif
