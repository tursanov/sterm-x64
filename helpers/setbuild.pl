#!/usr/bin/perl -w

## �����祭�� ����� ᡮન �ନ���� � 䠩�� build.h
## ��� ᮧ����� 䠩�� ������. (c) gsr, 2018.

$PROJ_NAME		= "������-2�-�";
$BUILD_HFILE_NAME	= "include/build.h";
$BUILD_HFILE_GUARD	= "BUILD_H";
$NR_BUILD		= -1;

if (open $f, "<", $BUILD_HFILE_NAME){
	while (<$f>){
		if (m/^\s*#define\s+STERM_VERSION_BUILD\s+(\d+)\s*$/){
			$NR_BUILD = $1;
			last;
		}
	}
	close $f;
}

open $f, ">", $BUILD_HFILE_NAME or die "�訡�� ������ $BUILD_HFILE_NAME ��� �����: $!.\n";

$NR_BUILD++;
chomp($date = `date "+%d.%m.%Y %H:%M:%S"`);

print $f <<EOF;
/* ����� ᡮન �ନ���� $PROJ_NAME ($date). */

#if !defined $BUILD_HFILE_GUARD
#define $BUILD_HFILE_GUARD

#define STERM_VERSION_BUILD	$NR_BUILD

#endif		/* $BUILD_HFILE_GUARD */
EOF
;

close $f;
