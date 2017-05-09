#!/bin/sh
rm -f data_*
gcc -lmraa -lm -o producer producer.c LSM9DS0.c
gcc -lmraa -lm -lfann -o consumer consumer.c
./producer &
PRODUCER_ID=$!
./consumer & 
CONSUMER_ID=$!
read -p $'Press any key to kill producer/consumer...\n' -n1 -s
kill -9 $PRODUCER_ID $CONSUMER_ID
