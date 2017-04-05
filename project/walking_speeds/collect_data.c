#include <stdio.h>
#include <string.h>
#include <mraa/i2c.h>
#include <unistd.h>
#include <signal.h>
#include "LSM9DS0.h"
#include <sys/time.h>

#define MILLION 1000000.0

sig_atomic_t volatile run_flag = 1;

void do_when_interrupted(int sig)
{
	if (sig == SIGINT)
		run_flag = 0;
}

int main() {
	//printf("test1\n");
	
	
	mraa_i2c_context accel, gyro;
	
	float ax, ay, az, gx, gy, gz, a_res, g_res;
	FILE *log;
	struct timeval start, end;
	double start_epoch, end_epoch;
	data_t accel_data, gyro_data, gyro_offset;
	accel_scale_t a_scale = A_SCALE_16G;
	gyro_scale_t g_scale = G_SCALE_2000DPS;

	accel = accel_init();
	set_accel_scale(accel, a_scale);
	set_accel_ODR(accel, A_ODR_1600);

	gyro = gyro_init();
	set_gyro_scale(gyro, g_scale);
	set_gyro_ODR(gyro, G_ODR_760_BW_100);
	
	a_res = calc_accel_res(a_scale);
	g_res = calc_gyro_res(g_scale);
	gyro_offset = calc_gyro_offset(gyro, g_res);
	
	//printf("test2\n");

	log = fopen("log.csv", "w");
	fprintf(log, "timestamp_before, timestamp_After, accel_x, accel_y, accel_z, gyro_x, gyro_y, gyro_z,\n");

	printf("Collecting data...\n");

	signal(SIGINT, do_when_interrupted);

	while(run_flag)
	{
		gettimeofday(&start, NULL);
		start_epoch = start.tv_sec + start.tv_usec/MILLION;
		
		accel_data = read_accel(accel, a_res);
		gyro_data = read_gyro(gyro, g_res);
		
		gettimeofday(&end, NULL);
		end_epoch = end.tv_sec + end.tv_usec/MILLION;
	
		ax = accel_data.x;
		ay = accel_data.y;
		az = accel_data.z;
		
		gx = gyro_data.x;
		gy = gyro_data.y;
		gz = gyro_data.z;
		
	
		fprintf(log, "%lf, %lf, %f, %f, %f, %f, %f, %f\n", start_epoch, end_epoch, ax, ay, az, gx, gy, gz);
		
		usleep(10);
	}	

	fclose(log);
	return 0;

}
