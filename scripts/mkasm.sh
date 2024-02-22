#!/bin/bash

## Создание ассемблерного файла на основании исходника на C.
## (c) gsr, 2020

CFLAGS="-std=gnu11 -Wall -Wextra -Wno-sign-compare -Wno-switch -pipe -pthread -O3 -DNDEBUG -Iinclude -D_GNU_SOURCE"

if [ ${1:-no} == "no" ]; then
	echo "use mkasm.sh file(s)"
else
	for f in "$@"; do
		echo "processing $f"
		out="${f/%.c/.asm}"
		[ "$out" != "$f" ] || out="$f.asm"
		gcc $CFLAGS -S $f -o $out
	done
fi
