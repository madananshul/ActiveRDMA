#!/bin/sh
ROOT=$1
RESULTS=$2
ACTIVE=$3

cp /usr/share/dict/words $ROOT/active--words
for x in 0 1 2 3 4 5 6 7 8 9; do
    cp /usr/share/dict/words $ROOT/words-$x
    sim/timing/timing > $RESULTS/t$x
    ./run-util.sh fsutils.Grep 10.0.0.2 $ACTIVE ^apple$ /active--words
    ./run-util.sh fsutils.Grep 10.0.0.2 $ACTIVE ^zoo$ /active--words
    ./run-util.sh fsutils.Grep 10.0.0.2 $ACTIVE ^moo$ /active--words
    sim/timing/timing >> $RESULTS/t$x
done 
