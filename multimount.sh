#!/bin/sh

base=$2

mkdir $base
for x in 0 1 2 3 4; do
    mkdir $base/$x
    SERVER=10.0.0.2 ACTIVE=$1 ./arfs_mount.sh $base/$x &
done

wait
