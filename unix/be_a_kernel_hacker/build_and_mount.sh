#!/bin/sh
make
gcc test.c -o test -std=gnu99
sudo rmmod heap
sudo insmod heap.ko _element_size=256