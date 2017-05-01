all: imu_data

imu_data: imu_data.c
	gcc -lmraa -lm -o imu_data imu_data.c LSM9DS0.c

clean:
	rm -rf imu_data *~
