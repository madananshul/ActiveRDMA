#!/bin/sh
export CLASSPATH=bin/:$CLASSPATH
exec java MainServer
