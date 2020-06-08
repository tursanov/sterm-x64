#!/usr/bin/perl -w

$DATE			= localtime(time);
$PROJ_NAME		= "������-2�-�";

$CONFIG_HFILE_NAME	= "include/config.h";
$CONFIG_HFILE_GUARD	= "CONFIG_H";

$CONFIG_SFILE_NAME	= "base/config.c";

$MAKE_CFG_NAME		= "Makefile.conf";

open (HFILE, ">$CONFIG_HFILE_NAME") or die "�訡�� ������ $CONFIG_HFILE_NAME: $!.\n";
open (SFILE, ">$CONFIG_SFILE_NAME") or die "�訡�� ������ $CONFIG_SFILE_NAME: $!.\n";
open (MAKE_CFG, ">$MAKE_CFG_NAME") or die "�訡�� ������ $MAKE_CFG_NAME: $!.\n";

# ��⠥� 䠩� ���䨣��樨
while (<>){
	s/\s*(#|;).*$//g;	# �ய�᪠�� �������ਨ
	s/^\n//g;		# �ய�᪠�� ����� ��ப�
	if (m/^\s*option\s+(\w+)/){
		$opt = $1;
		push(@options, $opt);
	}elsif (m/^\s*(\w+)\s+(.*$)/){
		$mk_options{$1}="$2";
		push(@mk_strings,$1);
	}
}

## �஢�ઠ ��ࠬ��஢
if (!exists($mk_options{VERSION_BRANCH}) ||
		!exists($mk_options{VERSION_RELEASE}) ||
		!exists($mk_options{VERSION_PATCH})){
	die "�� ������ 㪠���� ����� �ନ���� (BRANCH.RELEASE.PATCH).\n";
}

if (!exists($mk_options{STERM_HOME})){
	print "�� 㪠��� ��⠫�� �ନ����; �� 㬮�砭�� �㤥� �ᯮ�짮������ ⥪�騩.\n";
	$mk_options{STERM_HOME} = ".";
}elsif (!exists($mk_options{KDIR})){
	die "����室��� 㪠���� ��⠫�� � ��室�묨 ⥪�⠬� �� Linux (KDIR).\n";
}

$VERSION = sprintf("0x%.8x",
	(($mk_options{VERSION_BRANCH} & 0xff) << 16) |
	(($mk_options{VERSION_RELEASE} & 0xff) << 8) |
	(($mk_options{VERSION_PATCH} & 0xff)));

# ������� config.h
print HFILE <<EOF;
/*
 *	���� ���䨣��樨 �ନ���� \"$PROJ_NAME\".
 *	���� ᮧ������ ��⮬���᪨ � ������� configure.pl
 *	�� �᭮����� ������ �� .config.
 *	�६� ᮧ����� 䠩��: $DATE.
 */

#if !defined $CONFIG_HFILE_GUARD
#define $CONFIG_HFILE_GUARD

/* ����� �ନ���� */
#define STERM_VERSION\t\t$VERSION
#define STERM_VERSION_BRANCH\t$mk_options{VERSION_BRANCH}
#define STERM_VERSION_RELEASE\t$mk_options{VERSION_RELEASE}
#define STERM_VERSION_PATCH\t$mk_options{VERSION_PATCH}

#define VERSION_BRANCH(v) (((v) >> 16) & 0xff)
#define VERSION_RELEASE(v) (((v) >> 8) & 0xff)
#define VERSION_PATCH(v) ((v) & 0xff)

/* �����饭�� 䠩��� �ନ���� */
#define STERM_HOME\t\t"$mk_options{STERM_HOME}"

/* ��樨 �������樨 */
EOF
;

foreach $op (@options){
	print HFILE "#define $op\n";
}

print HFILE <<EOF;

extern void dump_config(void);

#endif		/* $CONFIG_HFILE_GUARD */
EOF
;

# ������� config.c
print SFILE <<EOF
/*
 *	���� ���䨣��樨 �ନ���� \"$PROJ_NAME\".
 *	���� ᮧ������ ��⮬���᪨ � ������� configure.pl
 *	�� �᭮����� ������ �� .config.
 *	�६� ᮧ����� 䠩��: $DATE.
 */

#include <stdio.h>
#include "build.h"
#include "sysdefs.h"

static const char *options[] = {
EOF
;

foreach $op (@options){
	print SFILE "\t\"$op\",\n";
}

print SFILE <<EOF
};

/* �뢮� ���ଠ樨 � ⥪�饩 ���䨣��樨 */
extern uint16_t term_check_sum;
void dump_config()
{
	int i;
	printf("$PROJ_NAME $mk_options{VERSION_BRANCH}.$mk_options{VERSION_RELEASE}.$mk_options{VERSION_PATCH}.%u (%.4hx)\\n"
		"��樨 �������樨:\\n", STERM_VERSION_BUILD, term_check_sum);
	for (i = 0; i < ASIZE(options); i++)
		printf("\\t%s\\n", options[i]);
}
EOF
;

# ������� Makefile.conf
print MAKE_CFG <<EOF
#	���� ���䨣��樨 ��� make
#	���� ᮧ������ ��⮬���᪨ � ������� configure.pl
#	�� �᭮����� ������ �� .config.
#	�६� ᮧ����� 䠩��: $DATE.

EOF
;

foreach $var (@mk_strings){
	print MAKE_CFG sprintf("%-24s= %s\n", $var, $mk_options{$var});
}

print MAKE_CFG "\n";

foreach $op (@options){
	print MAKE_CFG sprintf("%-24s= yes\n", $op);
}

print MAKE_CFG "\nCROSS_COMPILE =\n";

print MAKE_CFG <<EOF

CC                      = \$(CROSS_COMPILE)gcc
LD                      = \$(CROSS_COMPILE)ldd

BIN_GARBAGE		= \$(OBJS)

include Makefile.rules
EOF
;

close (HFILE);
close (SFILE);
close (MAKE_CFG);
