#!/bin/sh
export CLASSPATH=/usr/share/java/junit4.jar:bin:$CLASSPATH
java org.junit.runner.JUnitCore "$@"
