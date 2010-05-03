#!/bin/sh
ROOT=$1
RESULTS=$2

for x in 0 1 2 3 4 5 6 7 8 9; do
    cp -R ab $ROOT/ab$x
    sim/timing/timing > $RESULTS/t$x
    (cd $ROOT/ab$x; ./test.sh)
    sim/timing/timing >> $RESULTS/t$x
done
