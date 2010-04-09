#!/bin/sh
make clean
find . -name '*.o' -o -name '*.a' -exec rm {} \;
