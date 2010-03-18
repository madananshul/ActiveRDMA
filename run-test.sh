#!/bin/sh
export CLASSPATH=/usr/share/java/junit.jar:bin:$CLASSPATH
java org.junit.runner.JUnitCore "$@"
