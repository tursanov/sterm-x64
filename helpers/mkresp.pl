#!/usr/bin/perl

$XDIGIT="0-9a-fA-F";

while (<>){
	if (!m/(^#)|(^$)/){
		chomp;
		s/\\x([$XDIGIT]{2})/sprintf("%c", hex($1))/ge;
		tr/€–„…”ƒ•ˆ‰Š‹ŒŸ‘’“†‚œ›‡˜™—š/`abcdefghijklmnopqrstuvwxyz{|}~/;
		print;
	}
}
