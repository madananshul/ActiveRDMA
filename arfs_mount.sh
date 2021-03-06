#!/bin/bash

# Usage: ./arfs_mount.sh /mount/point

if [ "x$SERVER" == "x" ]; then
    SERVER=localhost
fi
if [ "x$ACTIVE" == "x" ]; then
    ACTIVE=active
fi

echo Server: $SERVER
echo Active: $ACTIVE

. ./build.conf

LD_LIBRARY_PATH=./jni:$FUSE_HOME/lib $JDK_HOME/bin/java \
   -classpath ./build:./lib/commons-logging-1.0.4.jar \
   -Dorg.apache.commons.logging.Log=fuse.logging.FuseLog \
   -Dfuse.logging.level=DEBUG \
   fuse.arfs.ARFilesystem $SERVER $ACTIVE -f -s $1
