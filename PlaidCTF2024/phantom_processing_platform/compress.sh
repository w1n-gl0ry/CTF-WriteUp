#!/bin/sh
#gcc -w -o exploit -static exploit.c -pthread -lrt &&\
# musl-gcc -w -s -static -o3 exploit.c -o exploit -masm=intel &&\
#mv exploit ./fs/ &&\
cd fs &&\
find . -print0 | cpio --owner root --null -ov --format=newc | gzip -9 > ../debug.cpio &&\
cd .. 
./run.sh
