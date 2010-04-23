#!/bin/sh
export CLASSPATH=/usr/share/java/junit4.jar:build:$CLASSPATH
java org.junit.runner.JUnitCore "$@"
