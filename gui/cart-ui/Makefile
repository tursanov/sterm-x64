include Makefile.conf
DIR = gui/cart-ui/

INCLUDE = ../../include
GUI_CART_OBJS =		\
	document \
	subcart	\
	cart \

OBJS = $(addsuffix .o, $(GUI_CART_OBJS))

all:		depend $(OBJS)

depend:
		@$(CC) $(CFLAGS) -MM $(OBJS:.o=.c) > .depend

clean:
		@rm -f $(BIN_GARBAGE)

ifneq ($(wildcard .depend),)
include .depend
endif
