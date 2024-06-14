MAKEFLAGS += -rR --no-print-directory
ifneq ($(wildcard Makefile.conf),)
include Makefile.conf
endif
include Makefile.rules

BASE_OBJS =		\
	base64		\
	cfg		\
	config		\
	devinfo		\
	ds1990a		\
	express		\
	gd		\
	genfunc		\
	hash		\
	iplir		\
	kbd		\
	keys		\
	licsig		\
	list		\
	md5		\
	numbers		\
	rndtbl		\
	serial		\
	serialize	\
	sterm		\
	termlog		\
	tki		\
	transport	\
	xbase64		\
	xchange		\
	xml2data

GUI_OBJS =		\
	adv_calc	\
	archivefn	\
	calc		\
	cheque		\
	cheque_docs	\
	cheque_reissue_docs	\
	dialog		\
	exgdi		\
	fa		\
	forms		\
	gdi		\
	help		\
	lvform		\
	menu		\
	options		\
	newcheque	\
	ping		\
	scr		\
	ssaver		\
	status		\
	xchange

GUI_LOG_OBJS =		\
	express		\
	generic		\
	kkt		\
	local		\
	pos

GUI_CONTROLS_OBJS =	\
	combobox	\
	control		\
	bitset		\
	button		\
	edit		\
	lang		\
	listbox		\
	listview	\
	window

KKT_OBJS =		\
	fdo		\
	io		\
	kkt		\
	parser		\
	xml

KKT_FD_OBJS =		\
	ad		\
	fd		\
	tags		\
	tlv

LOG_OBJS =		\
	express		\
	generic		\
	kkt		\
	local		\
	logdbg		\
	pos

POS_OBJS =		\
	command		\
	error		\
	pos		\
	printer		\
	screen		\
	serial		\
	tcp

PPP_OBJS =		\
	ppp

PRN_OBJS =		\
	aux		\
	express		\
	generic		\
	local

X3DATA_OBJS =		\
	common		\
	grids		\
	icons		\
	patterns	\
	xslts

OBJS =			\
	$(addprefix base/,	$(addsuffix .o, $(BASE_OBJS)))		\
	$(addprefix kkt/,	$(addsuffix .o, $(KKT_OBJS)))		\
	$(addprefix kkt/fd/,	$(addsuffix .o, $(KKT_FD_OBJS)))	\
	$(addprefix log/,	$(addsuffix .o, $(LOG_OBJS)))		\
	$(addprefix gui/,	$(addsuffix .o, $(GUI_OBJS)))		\
	$(addprefix gui/log/,	$(addsuffix .o, $(GUI_LOG_OBJS)))	\
	$(addprefix gui/controls/,	$(addsuffix .o, $(GUI_CONTROLS_OBJS)))	\
	$(addprefix pos/,	$(addsuffix .o, $(POS_OBJS)))		\
	$(addprefix ppp/,	$(addsuffix .o, $(PPP_OBJS)))		\
	$(addprefix prn/,	$(addsuffix .o, $(PRN_OBJS)))		\
	$(addprefix x3data/,	$(addsuffix .o, $(X3DATA_OBJS)))

SUBDIRS =		\
	base		\
	gui		\
	kkt		\
	log		\
	pos		\
	ppp		\
	prn		\
	x3data		\
	helpers		\
	scripts		\
	modules

.PHONY:	$(SUBDIRS)

all:	$(SUBDIRS) sterm mk_env


$(SUBDIRS):
	@$(MAKE) -C $@ -I../ -I../../

sterm:	$(OBJS)
	@echo "\t$(LD_NAME)   $@"
	@$(CC) $(CFLAGS) -o $@ $^ $(LINKAGE) -L /usr/lib/vipnet -lstdc++ -lvpn_api -lz -lxml2 -lxslt

mk_env:
	@if [ ! -f $(STERM_HOME)/sterm.dat ]; then\
		./helpers/mw --write-tki=$(STERM_HOME)/sterm.dat --number --chipset=cardless;\
	fi
	@./helpers/mw --read-tki=$(STERM_HOME)/sterm.dat --all

clean:
	@rm -f sterm
	@for dir in $(SUBDIRS); do \
		$(MAKE) clean -C $$dir -I../ -I../../; \
	done

more_clean:
	@rm -f	Makefile.conf \
		include/config.h \
		base/config.c
	@find . -name .depend -delete
	@find resp -name "*.rsp" -delete

dist_clean:	clean more_clean

CFG_DIR = "cfg"
BUILD_INFO = $(CFG_DIR)/build
RELEASE_CONFIG = $(CFG_DIR)/release
DEBUG_CONFIG = $(CFG_DIR)/debug

release-config:
	@helpers/configure.pl $(BUILD_INFO) $(RELEASE_CONFIG)

debug-config:
	@helpers/configure.pl $(BUILD_INFO) $(DEBUG_CONFIG)
