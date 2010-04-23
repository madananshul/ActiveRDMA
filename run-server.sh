#!/bin/sh
export CLASSPATH=build/:$CLASSPATH
exec java server.SimpleServer
