include Makefile.conf
DIR = gui/log/

INCLUDE = ../../include
GUI_LOG_OBJS =		\
	express		\
	generic		\
	kkt		\
	local		\
	pos

OBJS = $(addsuffix .o, $(GUI_LOG_OBJS))

all:		depend $(OBJS)

depend:
		@$(CC) $(CFLAGS) -MM $(OBJS:.o=.c) > .depend

clean:
		@rm -f $(BIN_GARBAGE)

ifneq ($(wildcard .depend),)
include .depend
endif
