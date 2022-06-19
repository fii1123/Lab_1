#!/bin/bash

mkdir tmp rez

cmake -S LisIf/ -B ./tmp/
make -C tmp
mv tmp/LisIf rez/

rm tmp/* -r

cmake -S LisIf_mon/ -B ./tmp/
make -C tmp
mv tmp/LisIf_mon rez/

rm tmp -r