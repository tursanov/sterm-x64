include Makefile.conf
DIR = scripts/

INCLUDE = ../include
CFLAGS += -D__STANDALONE__

BINARIES =		\
	bnklic		\
	dircmp		\
	dsd		\
	mwl		\
	rmbnklic	\
	rmbnklicx	\
	tpld		\
	tstbeep		\
	tstcom		\
	unlkbnk

BASE_OBJS =		\
	base64		\
	genfunc		\
	licsig		\
	md5		\
	numbers		\
	rndtbl

LOG_OBJS =		\
	generic		\
	express

SCRIPTS_OBJS =		\
	bnklic		\
	cfg		\
	dircmp		\
	ds1990a		\
	dsd		\
	mwl		\
	rmlic		\
	rmbnklic	\
	rmbnklicx	\
	serial		\
	tki		\
	tpld		\
	tstbeep		\
	tstcom		\
	unlkbnk		\
	unlock

DIR_BASE = ../base/
DIR_LOG = ../log/

OBJS =			\
	$(addprefix $(DIR_BASE), $(addsuffix .o, $(BASE_OBJS))) \
	$(addprefix $(DIR_LOG), $(addsuffix .o, $(LOG_OBJS))) \
	$(addsuffix .o, $(SCRIPTS_OBJS))

BIN_GARBAGE += $(BINARIES)

all:		depend $(BINARIES)

depend:
		@$(CC) $(CFLAGS) -MM $(OBJS:.o=.c) > .depend

## bnklic
BNKLIC_BASE_OBJS =	\
	base64		\
	licsig		\
	md5

BNKLIC_SCRIPTS_OBJS =	\
	bnklic		\
	tki

BNKLIC_OBJS =		\
	$(addprefix $(DIR_BASE), $(addsuffix .o, $(BNKLIC_BASE_OBJS))) \
	$(addsuffix .o, $(BNKLIC_SCRIPTS_OBJS))

bnklic:		$(BNKLIC_OBJS)
		$(LD_BANNER)
		@$(CC) $(CFLAGS) -o $@ $^ $(LINKAGE)

## dircmp

DIRCMP_SCRIPTS_OBJS =	\
	dircmp

DIRCMP_OBJS =		\
	$(addsuffix .o, $(DIRCMP_SCRIPTS_OBJS))

dircmp:		$(DIRCMP_OBJS)
		$(LD_BANNER)
		@$(CC) $(CFLAGS) -o $@ $^ $(LINKAGE)

## dsd
DSD_SCRIPTS_OBJS =	\
	dsd		\
	serial

DSD_OBJS =		\
	$(addsuffix .o, $(DSD_SCRIPTS_OBJS))

dsd:		$(DSD_OBJS)
		$(LD_BANNER)
		@$(CC) $(CFLAGS) -o $@ $^ $(LINKAGE)

## mwl
MWL_BASE_OBJS =		\
	base64		\
	licsig		\
	md5

MWL_SCRIPTS_OBJS =	\
	ds1990a		\
	mwl		\
	tki

MWL_OBJS =		\
	$(addprefix $(DIR_BASE), $(addsuffix .o, $(MWL_BASE_OBJS))) \
	$(addsuffix .o, $(MWL_SCRIPTS_OBJS))

mwl:		$(MWL_OBJS)
		$(LD_BANNER)
		@$(CC) $(CFLAGS) -o $@ $^ $(LINKAGE)

## rmbnklic
RMBNKLIC_BASE_OBJS =	\
	licsig		\
	md5		\
	rndtbl

RMBNKLIC_SCRIPTS_OBJS =	\
	rmbnklic	\
	rmlic

RMBNKLIC_OBJS =		\
	$(addprefix $(DIR_BASE), $(addsuffix .o, $(RMBNKLIC_BASE_OBJS))) \
	$(addsuffix .o, $(RMBNKLIC_SCRIPTS_OBJS))

rmbnklic:	$(RMBNKLIC_OBJS)
		$(LD_BANNER)
		@$(CC) $(CFLAGS) -o $@ $^ $(LINKAGE)

## rmbnklicx
RMBNKLICX_BASE_OBJS =	\
	base64		\
	licsig		\
	md5		\
	rndtbl

RMBNKLICX_SCRIPTS_OBJS =\
	rmbnklicx	\
	rmlic		\
	tki

RMBNKLICX_OBJS =	\
	$(addprefix $(DIR_BASE), $(addsuffix .o, $(RMBNKLICX_BASE_OBJS))) \
	$(addsuffix .o, $(RMBNKLICX_SCRIPTS_OBJS))

rmbnklicx:	$(RMBNKLICX_OBJS)
		$(LD_BANNER)
		@$(CC) $(CFLAGS) -o $@ $^ $(LINKAGE)

## tpld
TPLD_BASE_OBJS =	\
	numbers

TPLD_SCRIPTS_OBJS =	\
	cfg		\
	serial		\
	tpld

TPLD_OBJS =		\
	$(addprefix $(DIR_BASE), $(addsuffix .o, $(TPLD_BASE_OBJS))) \
	$(addsuffix .o, $(TPLD_SCRIPTS_OBJS))

tpld:		$(TPLD_OBJS)
		$(LD_BANNER)
		@$(CC) $(CFLAGS) -o $@ $^ $(LINKAGE)

## tstbeep
TSTBEEP_SCRIPTS_OBJS =	\
	serial		\
	tstbeep

TSTBEEP_OBJS =		\
	$(addsuffix .o, $(TSTBEEP_SCRIPTS_OBJS))

tstbeep:	$(TSTBEEP_OBJS)
		$(LD_BANNER)
		@$(CC) $(CFLAGS) -o $@ $^ $(LINKAGE)

## tstcom
TSTCOM_SCRIPTS_OBJS =	\
	serial		\
	tstcom

TSTCOM_OBJS =		\
	$(addsuffix .o, $(TSTCOM_SCRIPTS_OBJS))

tstcom:		$(TSTCOM_OBJS)
		$(LD_BANNER)
		@$(CC) $(CFLAGS) -o $@ $^ $(LINKAGE)

## unlkbnk
UNLKBNK_BASE_OBJS =	\
	licsig		\
	md5

UNLKBNK_SCRIPTS_OBJS =	\
	unlkbnk		\
	unlock

UNLKBNK_OBJS =		\
	$(addprefix $(DIR_BASE), $(addsuffix .o, $(UNLKBNK_BASE_OBJS))) \
	$(addsuffix .o, $(UNLKBNK_SCRIPTS_OBJS))

unlkbnk:	$(UNLKBNK_OBJS)
		$(LD_BANNER)
		@$(CC) $(CFLAGS) -o $@ $^ $(LINKAGE)

clean:
		@rm -f $(BIN_GARBAGE)

ifneq ($(wildcard .depend),)
include .depend
endif
