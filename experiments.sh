#!/bin/bash
ITERATIONS=3000
SEED=144

for P in 2 3 4 5
do
    for systemtype in serialparallel
    do
        for n in 100 500 1000 1500 2000 2500 3000
        do
            ./build/release/experiment $ITERATIONS $SEED $P $systemtype one $n >> "results/p${P}_${systemtype}_one_n${n}.csv"
            ./build/release/experiment $ITERATIONS $SEED $P $systemtype multiple $n >> "results/p${P}_${systemtype}_multiple_n${n}.csv"
        done
    done
done
