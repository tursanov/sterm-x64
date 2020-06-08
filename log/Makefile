include Makefile.conf
DIR = log/

INCLUDE = ../include
LOG_OBJS =		\
	express		\
	generic		\
	kkt		\
	local		\
	logdbg		\
	pos

OBJS = $(addsuffix .o, $(LOG_OBJS))

all:		depend $(OBJS)

depend:
		@$(CC) $(CFLAGS) -MM $(OBJS:.o=.c) > .depend

clean:
		@rm -f $(BIN_GARBAGE)

ifneq ($(wildcard .depend),)
include .depend
endif
