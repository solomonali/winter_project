#!/bin/bash

if [ $1 ]; then
		threshold=$1
else
		threshold=6
fi

echo ""
echo ">>>>COMPILING<<<<"
make clean
make


echo ""
echo ">>>>EXECUTING extract_stride_data on ACCELERATION DATASET<<<<"
./extract_stride_data \
	data.csv \
	pt_output.csv \
	strides.csv \
	$threshold

echo ""
echo "Lines in Picks and troughs  output.csv"
wc -l pt_output.csv


echo ""
echo "Lines in strides.csv"
wc -l strides.csv

echo ""
echo "Number of lines in train_set.txt"
wc -l train_set.txt




