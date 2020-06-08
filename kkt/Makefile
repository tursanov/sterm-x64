include Makefile.conf
DIR = kkt/

INCLUDE = ../include

KKT_OBJS =		\
	fdo		\
	io		\
	kkt		\
	parser

OBJS = $(addsuffix .o, $(KKT_OBJS))

SUBDIRS =		\
	fd		\
	xml

.PHONY: $(SUBDIRS)

all:	depend $(OBJS) $(SUBDIRS)

$(SUBDIRS):
		@$(MAKE) -C $@ -I../../

depend:
		@$(CC) $(CFLAGS) -MM $(OBJS:.o=.c) > .depend
		@for dir in $(SUBDIRS); do \
			$(MAKE) depend -C $$dir -I../../; \
		done

clean:
		@rm -f $(BIN_GARBAGE)
		@for dir in $(SUBDIRS); do \
			$(MAKE) clean -C $$dir -I../../; \
		done

ifneq ($(wildcard .depend),)
include .depend
endif
