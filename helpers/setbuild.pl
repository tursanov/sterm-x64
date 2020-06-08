#!/usr/bin/perl -w

## Увеличение номера сборки терминала в файле build.h
## или создание файла заново. (c) gsr, 2018.

$PROJ_NAME		= "Экспресс-2А-К";
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

open $f, ">", $BUILD_HFILE_NAME or die "Ошибка открытия $BUILD_HFILE_NAME для записи: $!.\n";

$NR_BUILD++;
chomp($date = `date "+%d.%m.%Y %H:%M:%S"`);

print $f <<EOF;
/* Номер сборки терминала $PROJ_NAME ($date). */

#if !defined $BUILD_HFILE_GUARD
#define $BUILD_HFILE_GUARD

#define STERM_VERSION_BUILD	$NR_BUILD

#endif		/* $BUILD_HFILE_GUARD */
EOF
;

close $f;
