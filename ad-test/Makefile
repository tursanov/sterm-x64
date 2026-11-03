include Makefile.conf
DIR = ad-test/

INCLUDE = ../include

AD_TEST_OBJS =		\
	adtest

OBJS = $(addsuffix .o, $(AD_TEST_OBJS))

all:		depend $(OBJS)

depend:
		@$(CC) $(CFLAGS) -MM $(addsuffix .c, $(AD_TEST_OBJS)) > .depend

clean:
		@rm -f $(BIN_GARBAGE)

ifneq ($(wildcard .depend),)
include .depend
endif
