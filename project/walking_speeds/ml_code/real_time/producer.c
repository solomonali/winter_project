#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <signal.h>
#include <mraa/i2c.h>
#include "LSM9DS0.h"

#define MILLION 1000000.0
#define SAMPLES 1500

int main()
{
    FILE *fp;
    char filename[50];
    
    int i, filenum = 0, sigma = 0;
	
    int fd;

    mraa_i2c_context accel, gyro;
    float a_res, g_res;
    double t, start_epoch, end_epoch;
    struct timeval start, end;
    
    data_t accel_data, gyro_data;
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

    while (1) {

	sprintf(filename, "data_%d.txt", filenum);
    	fp = fopen(filename, "w");
	
	if (fp == NULL)
	{
		fprintf(stderr, "Failed to write to file.\n");
		exit(EXIT_FAILURE);
	}

	//printf("Filenum: %d, Filename: %s\n", filenum, filename);

	fd = fileno(fp);
	flock(fd, LOCK_EX);

	for (i = 0; i < SAMPLES; i++)
	{
		gettimeofday(&start, NULL);
		start_epoch = start.tv_sec + start.tv_usec/MILLION;
		
		accel_data = read_accel(accel, a_res);
		gyro_data = read_gyro(gyro, g_res);
		
		gettimeofday(&end, NULL);
		end_epoch = end.tv_sec + end.tv_usec/MILLION;
	
		t = (start_epoch + end_epoch)/2.0;

		fprintf(fp, "%d,%lf,%f,%f,%f,%f,%f,%f\n", i+SAMPLES*filenum, t, accel_data.x, accel_data.y, accel_data.z, gyro_data.x, gyro_data.y, gyro_data.z);
		
		usleep(99);
	}

	//printf("Next window\n");
	fclose(fp);
	filenum++;
    }

    return 0;
}

void adjust_window(float *arr, int sigma)
{
	int i;
	for(i = 0; i < sigma; i++)
	{
		arr[i] = arr[SAMPLES - sigma + i];
	}
	return;
}
