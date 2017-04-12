#include <unistd.h>

#include <stdio.h>
#include "floatfann.h"

#include <string.h>
#include <stdlib.h>

#include <sys/time.h>
#include <signal.h>
#include <mraa/i2c.h>
#include "LSM9DS0.h"

#define MILLION 1000000.0
#define SAMPLES 6000

sig_atomic_t volatile run_flag = 1;

void do_when_interrupted(int sig)
{
	if (sig == SIGINT)
		run_flag = 0;
}

int find_peaks_and_troughs(float *arr,int n_samples, float E, float *P, float *T, int *n_P, int *n_T);
void clear_buffer(float *arr, float val, int n);

int main()
{
    int i, j, idx, idx_min, idx_next,  val;
    int speed;
    int n_S, n_P, n_T;
    fann_type *calc_out;
    fann_type input[3];
    struct fann *ann;
   
    ann = fann_create_from_file("TEST.net");
	
    mraa_i2c_context accel, gyro;
    float *min, *max, fmax, a_res, g_res, threshold=1.7;
    float ax[SAMPLES], ay[SAMPLES], az[SAMPLES], gx[SAMPLES], gy[SAMPLES], gz[SAMPLES];
    float *P_i, *T_i, *S_i, *S_min;
    double t[SAMPLES], start_epoch, end_epoch, *periods, period;
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


    signal(SIGINT, do_when_interrupted);

    while (run_flag) {
	 
	clear_buffer(ax, 0.0f, SAMPLES);
	for (i = 0; i < SAMPLES; i++)
	{
		gettimeofday(&start, NULL);
		start_epoch = start.tv_sec + start.tv_usec/MILLION;
		
		accel_data = read_accel(accel, a_res);
		gyro_data = read_gyro(gyro, g_res);
		
		gettimeofday(&end, NULL);
		end_epoch = end.tv_sec + end.tv_usec/MILLION;

	
		ax[i] = accel_data.x;
		ay[i] = accel_data.y;
		az[i] = accel_data.z;

		gx[i] = gyro_data.x;
		gy[i] = gyro_data.y;
		gz[i] = gyro_data.z;
	
		t[i] = (start_epoch + end_epoch)/2.0;

		//printf("i: %d\t t: %lf\t ax: %f\t ay: %f\t az: %f\t gx: %f\t gy: %f\t gz: %f\n", i, t[i], ax[i], ay[i], az[i], gx[i], gy[i], gz[i]);
		
		usleep(99);
	}
	

	//printf("Finding peaks and troughs...\n");

    	P_i = (float *) malloc(sizeof(float) * SAMPLES);
    	T_i = (float *) malloc(sizeof(float) * SAMPLES);
	val = find_peaks_and_troughs(ax, SAMPLES, threshold, P_i, T_i, &n_P, &n_T);
	
	if (val < 0) {
		fprintf(stderr, "find_peaks_and_throughs failed\n");
		exit(EXIT_FAILURE);
	}

	S_i = (float *) malloc(sizeof(float) * n_P); // P
	S_min = (float *) malloc(sizeof(float) * n_T); // T

	n_S = detect_strides(ax, S_i, S_min, P_i, T_i, t, n_P, n_T);
	
	periods = (double *) malloc(sizeof(double) * n_S);
    	min = (float *) malloc(sizeof(float) * n_S);
    	max = (float *) malloc(sizeof(float) * n_S);
	
	for (i = 0; i < n_S; i++) {
		idx = (int) S_i[i];
		idx_min = (int) S_min[i];
		idx_next = (int) S_i[i+1];
		((i+1)!=n_S)? period = t[idx_next]- t[idx]: period;
		periods[i] = period/10.0;
		max[i] = ax[idx]/10.0;
		min[i] = ax[idx_min]/10.0;
	}

	//determine walking speed for each stride
	for (j = 0; j < n_S; j++)
	{
		fmax = -100;   
	        input[0] = (float) periods[j];
        	input[1] = (float) max[j];
	        input[2] = (float) min[j];
        	calc_out = fann_run(ann, input);

	        for (i = 0; i < 4; i++) {
         	   if (calc_out[i] > fmax) {
                	fmax = calc_out[i];
	                speed = i+1;
        	    }
       		}
		printf("Period: %f\t Max: %f\t Min: %f\t -> speed is %d\n", periods[j], max[j], min[j], speed);
		sleep(1);
	}

    }
    fann_destroy(ann);
    return 0;
}

int find_peaks_and_troughs(float *arr,int n_samples, float E, float *P, float *T, int *n_P, int *n_T)
{
	int a, b, i, d, _n_P, _n_T;

	i = -1; d = 0; a = 0; b = 0;
	_n_P = 0; _n_T = 0;

//	printf("Clearing buffer...\n");

	clear_buffer(P, 0.0f, SAMPLES);
	clear_buffer(T, 0.0f, SAMPLES);

	while (i != SAMPLES) {
		i++;
		if (d == 0) {
			if (arr[a] >= (arr[i] + E)) {
				d = 2;
			} else if (arr[i] >= (arr[b] + E)) {
				d = 1;
			}
			if (arr[a] <= arr[i]) {
				a = i;
			} else if (arr[i] <= arr[b]) {
				b = i;
			}
		} else if (d == 1) {
			if (arr[a] <= arr[i]) {
				a = i;
			} else if (arr[a] >= (arr[i] + E)) {
				/*
				 * Peak has been detected.
				 * Add index at detected peak
				 * to array of peak indicies
				 * increment count of peak indicies
				 */
				P[_n_P] = a;
				_n_P++;
				b = i;
				d = 2;
			}
		} else if (d == 2) {
			if (arr[i] <= arr[b]) {
				b = i;
			} else if (arr[i] >= (arr[b] + E)) {
				/*
				 * Trough has been detected.
				 * Add index at detected trough
				 * to array of trough indicies
				 * increment count of trough indicies
				 */
				T[_n_T] = b;
				_n_T++;
				a = i;
				d = 1;
			}
		}
	}
	
	//printf("Peaks and troughs found...\n");

	(*n_P) = _n_P;
	(*n_T) = _n_T;
	return 0;
}

int detect_strides(float *arr, float *S_i, float *S_min, float *P_i, float *T_i, double *t, int n_P, int n_T)
{
  	int a, b, c, i, idx, n_S;
	a = 0; b = 1; n_S = 0; c = 0; 
	while( b < n_P)                     //P
	{
		if((P_i[b] - P_i[a]) > 300)   //P
		{
			S_i[n_S] = P_i[a];         //P
		        S_min[n_S] = T_i[a];	  // T
			n_S++;
			a=a+c+1;              
			b=b+1;
			c=0;
			/*checking for boundary conditions: if the last stride*/ 
			if((b == n_P) && ((P_i[b-1] - P_i[a-1])>300)) //P
		        {
				S_i[n_S] = P_i[b-1];	//P
				S_min[n_S] = T_i[b-1];  //T
				n_S++;
			}	
		}
		else{
			b++;
			c++;
                        /*checking for boundary conditions: if the last stride*/
			if((b == n_P) && ((P_i[a] - P_i[a-1])>300))  //P
			{		
				S_i[n_S] = P_i[a];   //P
                        	S_min[n_S] = T_i[a];  //T
				n_S++;
			}	
		}
	
	}

	return n_S;
}

void clear_buffer(float *arr, float val, int n)
{
	int i;
	for (i = 0; i < n; i++)
		arr[i] = val;
}

float calculate_mean(float *arr, int n)
{
	float total;
	int i;

	total = 0.0f;
	for (i = 0; i < n; i++)
		total += arr[i];

	return total/((float) n);
}

