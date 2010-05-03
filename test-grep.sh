#!/bin/sh
ROOT=$1
RESULTS=$2

for x in 0 1 2 3 4 5 6 7 8 9; do
    cp /usr/share/dict/words $ROOT/words-$x
    sim/timing/timing > $RESULTS/t$x
    grep ^apple$ $ROOT/words-$x
    grep ^zoo$ $ROOT/words-$x
    grep ^moo$ $ROOT/words-$x
    sim/timing/timing >> $RESULTS/t$x
done 
