#!/bin/sh
mkdir test
cd test
make -f ../Makefile
cd ..
rm -rf test
