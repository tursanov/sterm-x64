include Makefile.conf
DIR = kkt/fd/

INCLUDE = ../../include
FFD_OBJS =		\
	ad 		\
	fd		\
	tags	\
	tlv     \
	pattern

OBJS = $(addsuffix .o, $(FFD_OBJS))

all:		depend $(OBJS)

depend:
		@$(CC) $(CFLAGS) -MM $(OBJS:.o=.c) > .depend

clean:
		@rm -f $(BIN_GARBAGE)

ifneq ($(wildcard .depend),)
include .depend
endif
