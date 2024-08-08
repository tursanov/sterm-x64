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
CART = cart-ui
LOG = log

.PHONY:	$(LOG) $(CONTROLS) $(CART)

all:		depend $(OBJS) $(LOG) $(CONTROLS) $(CART)

$(LOG):
		@$(MAKE) -C $@ -I../../

$(CONTROLS):
		@$(MAKE) -C $@ -I../../

$(CART):
		@$(MAKE) -C $@ -I../../

depend:
		@$(CC) $(CFLAGS) -MM $(OBJS:.o=.c) > .depend
		@$(MAKE) depend -C $(LOG) -I../../
		@$(MAKE) depend -C $(CONTROLS) -I../../
		@$(MAKE) depend -C $(CART) -I../../

clean:
		@rm -f $(BIN_GARBAGE)
		@$(MAKE) clean -C log -I../../
		@$(MAKE) clean -C controls -I../../
		@$(MAKE) clean -C cart-ui -I../../

ifneq ($(wildcard .depend),)
include .depend
endif
