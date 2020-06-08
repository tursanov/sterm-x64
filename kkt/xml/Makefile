include Makefile.conf
DIR = kkt/xml/

INCLUDE = ../../include
XML_OBJS =		\
	xml

OBJS = $(addsuffix .o, $(XML_OBJS))

all:		depend $(OBJS)

depend:
		@$(CC) $(CXXFLAGS) -MM $(OBJS:.o=.cpp) > .depend

clean:
		@rm -f $(BIN_GARBAGE)

ifneq ($(wildcard .depend),)
include .depend
endif
