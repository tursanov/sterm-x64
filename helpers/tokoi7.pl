#!/usr/bin/perl

$XDIGIT="0-9a-fA-F";

while (<>){
	if (!m/(^#)|(^$)/){
		tr/��������������������������������/`abcdefghijklmnopqrstuvwxyz{|}~/;
		print;
	}
}
