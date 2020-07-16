#!/bin/bash

app=vpntest

echo -n compiling...
if gcc -Wall -Wextra -std=gnu11 -g -L/usr/lib/vipnet -lvpn_api -lstdc++ -pthread -o $app ${app}.c; then
	echo ok.
##	echo -n stripping...
##	strip $app && echo ok.
fi

