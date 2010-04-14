#!/bin/sh

#
# Usage: ./fakefs_mount.sh /mount/point -f [ -s ]

. ./build.conf

LD_LIBRARY_PATH=./jni:$FUSE_HOME/lib $JDK_HOME/bin/java \
   $JAVA_OPTS \
   -classpath ./build:./lib/commons-logging-1.0.4.jar \
   -Dorg.apache.commons.logging.Log=fuse.logging.FuseLog \
   -Dfuse.logging.level=DEBUG \
   -Dcom.sun.management.jmxremote \
   fuse.FakeFilesystem $*
