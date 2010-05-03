#!/bin/sh
ROOT=$1
RESULTS_READ=$2
RESULTS_WRITE=$3

for x in 0 1 2 3 4 5 6 7 8 9; do
    sim/timing/timing > $RESULTS_WRITE/t$x
    dd if=/dev/zero of=$ROOT/stream$x bs=5M count=1
    sim/timing/timing >> $RESULTS_READ/t$x
done

for x in 0 1 2 3 4 5 6 7 8 9; do
    sim/timing/timing > $RESULTS_READ/t$x
    dd if=$ROOT/stream$x of=empty_file bs=5M count=1
    sim/timing/timing >> $RESULTS_READ/t$x
done

rm empty_file
