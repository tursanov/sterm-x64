include Makefile.conf
DIR = gui/controls/

INCLUDE = ../../include
GUI_CONTROLS_OBJS =		\
	combobox \
	control	\
	bitset \
	button \
	edit \
	lang \
	listbox \
	listview \
	window \


OBJS = $(addsuffix .o, $(GUI_CONTROLS_OBJS))

all:		depend $(OBJS)

depend:
		@$(CC) $(CFLAGS) -MM $(OBJS:.o=.c) > .depend

clean:
		@rm -f $(BIN_GARBAGE)

ifneq ($(wildcard .depend),)
include .depend
endif
