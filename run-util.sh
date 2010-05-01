#!/bin/sh
export CLASSPATH=build:$CLASSPATH
java "$@"
