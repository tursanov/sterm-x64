include Makefile.conf
DIR = gui/

INCLUDE = ../include

GUI_OBJS =		\
	adv_calc	\
	archivefn	\
	calc		\
	cheque		\
	cheque_docs \
	cheque_reissue_docs \
	dialog		\
	exgdi		\
	fa		\
	forms		\
	gdi		\
	help		\
	lvform	\
	menu		\
	newcheque	\
	options		\
	ping		\
	scr		\
	ssaver		\
	status		\
	xchange

OBJS = $(addsuffix .o, $(GUI_OBJS))

CONTROLS = controls
LOG = log

.PHONY:	$(LOG) $(CONTROLS)

all:		depend $(OBJS) $(LOG) $(CONTROLS)

$(LOG):
		@$(MAKE) -C $@ -I../../

$(CONTROLS):
		@$(MAKE) -C $@ -I../../

depend:
		@$(CC) $(CFLAGS) -MM $(OBJS:.o=.c) > .depend
		@$(MAKE) depend -C $(LOG) -I../../
		@$(MAKE) depend -C $(CONTROLS) -I../../

clean:
		@rm -f $(BIN_GARBAGE)
		@$(MAKE) clean -C log -I../../
		@$(MAKE) clean -C controls -I../../

ifneq ($(wildcard .depend),)
include .depend
endif
