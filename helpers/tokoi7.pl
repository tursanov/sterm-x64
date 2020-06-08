#!/usr/bin/perl

$XDIGIT="0-9a-fA-F";

while (<>){
	if (!m/(^#)|(^$)/){
		tr/€–„…”ƒ•ˆ‰Š‹ŒŸ‘’“†‚œ›‡˜™—š/`abcdefghijklmnopqrstuvwxyz{|}~/;
		print;
	}
}
