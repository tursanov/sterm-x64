include Makefile.conf
DIR = pos/

INCLUDE = ../include

POS_OBJS =		\
	command		\
	error		\
	pos		\
	printer		\
	screen		\
	serial		\
	tcp

OBJS = $(addsuffix .o, $(POS_OBJS))

all:		depend $(OBJS)

depend:
		@$(CC) $(CFLAGS) -MM $(OBJS:.o=.c) > .depend

clean:
		@rm -f $(BIN_GARBAGE)

ifneq ($(wildcard .depend),)
include .depend
endif
