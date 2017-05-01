#!/bin/bash

echo ""
echo ">>>>COMPILING<<<<"

rm file*
make clean
make


echo ""
echo ">>>>EXECUTING imu_data<<<<"
./imu_data &
imu_process=$!

echo ""
echo ">>>>executing extract_stride_data<<<<"
./extract_features_data &
extract_process=$!	

#echo ""
#echo ">>>>executing extract_stride_data<<<<"
#./test_neural_network &
 
read -p "press anything to continue..." -n1 -s

echo "-------------"
kill $imu_process
kill $extract_process
