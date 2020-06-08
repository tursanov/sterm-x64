include Makefile.conf
DIR = helpers/

#CC = gcc

INCLUDE = ../include

BINARIES =		\
	chipid		\
	chkrmlic	\
	mkbind		\
	mkbnklic	\
	mklprnlic	\
	mklog		\
	mw		\
	usbload

BASE_OBJS =		\
	base64		\
	ds1990a		\
	genfunc		\
	md5		\
	rndtbl		\
	tki

HELPERS_OBJS =		\
	chipid		\
	chkrmlic	\
	licsig		\
	mkbind		\
	mkbnklic	\
	mklic		\
	mklog		\
	mklprnlic	\
	mw		\
	usbload

USB_OBJS =		\
	core

DIR_BASE = ../base/
DIR_USB = ../usb/

OBJS =			\
	$(addprefix $(DIR_BASE), $(addsuffix .o, $(BASE_OBJS))) \
	$(addsuffix .o, $(HELPERS_OBJS)) \
	$(addprefix $(DIR_USB), $(addsuffix .o, $(USB_OBJS)))

BIN_GARBAGE += $(BINARIES)

all:		depend $(BINARIES)

depend:
		@$(CC) $(CFLAGS) -MM $(OBJS:.o=.c) > .depend

chipid:		chipid.o
		@echo "\t$(LD_NAME)   $@"
		@$(CC) $(CFLAGS) -o $@ $^ $(LINKAGE)

## chkrmlic
CHKRMLIC_BASE_OBJS =	\
	base64		\
	md5		\
	rndtbl		\
	tki

CHKRMLIC_HELPERS_OBJS =	\
	chkrmlic	\
	licsig

CHKRMLIC_OBJS =		\
	$(addprefix $(DIR_BASE), $(addsuffix .o, $(CHKRMLIC_BASE_OBJS))) \
	$(addsuffix .o, $(CHKRMLIC_HELPERS_OBJS))

chkrmlic:	$(CHKRMLIC_OBJS)
		@echo "\t$(LD_NAME)   $@"
		@$(CC) $(CFLAGS) -o $@ $^

## mkbind
MKBIND_BASE_OBJS =	\
	base64		\
	md5		\
	tki

MKBIND_HELPERS_OBJS =	\
	licsig		\
	mkbind

MKBIND_OBJS =		\
	$(addprefix $(DIR_BASE), $(addsuffix .o, $(MKBIND_BASE_OBJS))) \
	$(addsuffix .o, $(MKBIND_HELPERS_OBJS))

mkbind:		$(MKBIND_OBJS)
		@echo "\t$(LD_NAME)   $@"
		@$(CC) $(CFLAGS) -o $@ $^

## mkbnklic
MKBNKLIC_BASE_OBJS =	\
	base64		\
	md5		\
	tki

MKBNKLIC_HELPERS_OBJS =	\
	licsig		\
	mkbnklic	\
	mklic

MKBNKLIC_OBJS =	\
	$(addprefix $(DIR_BASE), $(addsuffix .o, $(MKBNKLIC_BASE_OBJS))) \
	$(addsuffix .o, $(MKBNKLIC_HELPERS_OBJS))

mkbnklic:	$(MKBNKLIC_OBJS)
		@echo "\t$(LD_NAME)   $@"
		@$(CC) $(CFLAGS) -o $@ $^

## mklog
MKLOG_BASE_OBJS =	\
	genfunc

MKLOG_HELPERS_OBJS =	\
	mklog

MKLOG_OBJS =		\
	$(addprefix $(DIR_BASE), $(addsuffix .o, $(MKLOG_BASE_OBJS))) \
	$(addsuffix .o, $(MKLOG_HELPERS_OBJS))

mklog:		$(MKLOG_OBJS)
		@echo "\t$(LD_NAME)   $@"
		@$(CC) $(CFLAGS) -o $@ $^

## mklprnlic
MKLPRNLIC_BASE_OBJS =	\
	base64		\
	md5		\
	tki

MKLPRNLIC_HELPERS_OBJS =\
	licsig		\
	mklic		\
	mklprnlic

MKLPRNLIC_OBJS =	\
	$(addprefix $(DIR_BASE), $(addsuffix .o, $(MKLPRNLIC_BASE_OBJS))) \
	$(addsuffix .o, $(MKLPRNLIC_HELPERS_OBJS))

mklprnlic:	$(MKLPRNLIC_OBJS)
		@echo "\t$(LD_NAME)   $@"
		@$(CC) $(CFLAGS) -o $@ $^

## mw
MW_BASE_OBJS =		\
	base64		\
	ds1990a		\
	genfunc		\
	md5		\
	tki

MW_HELPERS_OBJS =	\
	licsig		\
	mw

MW_OBJS =		\
	$(addprefix $(DIR_BASE), $(addsuffix .o, $(MW_BASE_OBJS))) \
	$(addsuffix .o, $(MW_HELPERS_OBJS))

mw:		$(MW_OBJS)
		@echo "\t$(LD_NAME)   $@"
		@$(CC) $(CFLAGS) -o $@ $^ -static

## usbload
USBLOAD_BASE_OBJS =	\
	md5

USBLOAD_USB_OBJS =	\
	core

USBLOAD_HELPERS_OBJS =	\
	usbload

USBLOAD_OBJS =		\
	$(addprefix $(DIR_BASE), $(addsuffix .o, $(USBLOAD_BASE_OBJS))) \
	$(addprefix $(DIR_USB), $(addsuffix .o, $(USBLOAD_USB_OBJS))) \
	$(addsuffix .o, $(USBLOAD_HELPERS_OBJS))

usbload:	$(USBLOAD_OBJS)
		@echo "\t$(LD_NAME)   $@"
		@$(CC) $(CFLAGS) -o $@ $^ $(LINKAGE)

clean:
		@rm -f $(BIN_GARBAGE)

ifneq ($(wildcard .depend),)
include .depend
endif
