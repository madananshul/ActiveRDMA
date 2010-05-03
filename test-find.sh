#!/bin/sh
ROOT=$1
RESULTS=$2

for x in 0 1 2 3 4 5 6 7 8 9; do
    cp -R sim/find-testtree $ROOT/find--$x
    sim/timing/timing > $RESULTS/t$x
    (cd $ROOT/find--$x; find . -name 'sb16ctrl.c')
    sim/timing/timing >> $RESULTS/t$x
done 
