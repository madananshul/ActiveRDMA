#!/bin/sh
JVM=/usr/lib/jvm/java-1.5.0-gcj-4.4

g++ -g java.cc -I$JVM/include -L$JVM/lib -ljvm -Wl,-rpath $JVM/lib -o j
