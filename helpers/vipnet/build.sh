#!/bin/bash

app=vpntest

echo -n compiling...
if gcc -Wall -Wextra -std=gnu11 -O3 -L/usr/lib/vipnet -lvpn_api -o $app ${app}.c; then
	echo ok.
	echo -n stripping...
	strip $app && echo ok.
fi

