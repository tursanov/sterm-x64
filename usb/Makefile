include Makefile.conf
DIR = usb/

INCLUDE = ../include
USB_OBJS =		\
	core		\
	key

OBJS = $(addsuffix .o, $(USB_OBJS))

all:		depend $(OBJS)

depend:
		@$(CC) $(CFLAGS) -MM $(OBJS:.o=.c) > .depend

clean:
		@rm -f $(BIN_GARBAGE)

ifneq ($(wildcard .depend),)
include .depend
endif
