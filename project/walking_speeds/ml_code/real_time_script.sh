#!/bin/sh
gcc -lmraa -lm -o producer producer.c LSM9DS0.c
gcc -lmraa -lm -lfann -o consumer consumer.c
./producer &
./consumer &
