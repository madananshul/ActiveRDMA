#!/bin/sh
ROOT=$1
RESULTS=$2
ACTIVE=$3

cp -R sim/find-testtree $ROOT/active--find
for x in 0 1 2 3 4 5 6 7 8 9; do
    sim/timing/timing > $RESULTS/t$x
    ./run-util.sh fsutils.Find 10.0.0.2 $ACTIVE '.*sb16ctrl.c'
    sim/timing/timing > $RESULTS/t$x
done 
