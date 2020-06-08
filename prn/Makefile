include Makefile.conf
DIR = prn/

INCLUDE = ../include
LOG_OBJS =		\
	aux		\
	express		\
	generic		\
	local

OBJS = $(addsuffix .o, $(LOG_OBJS))

all:		depend $(OBJS)

depend:
		@$(CC) $(CFLAGS) -MM $(OBJS:.o=.c) > .depend

clean:
		@rm -f $(BIN_GARBAGE)

ifneq ($(wildcard .depend),)
include .depend
endif
