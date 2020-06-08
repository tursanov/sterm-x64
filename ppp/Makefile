include Makefile.conf
DIR = ppp/

INCLUDE = ../include

PPP_OBJS =		\
	ppp

OBJS = $(addsuffix .o, $(PPP_OBJS))

all:		depend $(OBJS)

depend:
		@$(CC) $(CFLAGS) -MM $(OBJS:.o=.c) > .depend

clean:
		@rm -f $(BIN_GARBAGE)

ifneq ($(wildcard .depend),)
include .depend
endif
