#!/bin/sh
rm -f data_*
./producer &
PRODUCER_ID=$!
./consumer & 
CONSUMER_ID=$!
read -p $'Press any key to kill producer/consumer...\n' -n1 -s
kill -9 $PRODUCER_ID $CONSUMER_ID
