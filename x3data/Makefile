include Makefile.conf
DIR = x3data/

INCLUDE = ../include

X3DATA_OBJS =		\
	grids		\
	icons		\
	patterns	\
	xslts

OBJS = $(addsuffix .o, $(X3DATA_OBJS))

all:		depend $(OBJS)

depend:
		@$(CC) $(CXXFLAGS) -MM $(OBJS:.o=.cpp) > .depend

clean:
		@rm -f $(BIN_GARBAGE)

ifneq ($(wildcard .depend),)
include .depend
endif
