CC=gcc
LDFLAGS=-lmraa -lfann
CFLAGS=-Wall
SOURCES= test_neural_network.c train_neural_net.c
EXECUTABLES=$(SOURCES:.c=)

all: extract_features_data  imu_data test_neural_network train_neural_net

imu_data: ./stage1/imu_data.c
	gcc -lmraa -lm -o imu_data ./stage1/imu_data.c ./stage1/LSM9DS0.c


extract_features_data: ./stage2/extract_features_data.c
	gcc $(CFLAGS) -o extract_features_data ./stage2/extract_features_data.c -lm $(LDFLAGS)


test_neural_network: ./stage3/test_neural_network.c
	$(CC) $(CFLAGS) -o test_neural_network ./stage3/test_neural_network.c $(LDFLAGS)

train_neural_net: ./stage3/train_neural_net.c
	$(CC) $(CFLAGS) -o train_neural_net ./stage3/train_neural_net.c $(LDFLAGS)

clean: 	
	rm -rf imu_data *~
	rm -rf extract_features_data *~
	rm -f test_neural_network train_neural_net
	rm -f *~
	rm -f TEST.net
