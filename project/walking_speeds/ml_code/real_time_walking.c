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
#define SAMPLES 2000


int find_peaks_and_troughs(float *arr,int n_samples, float E, float *P, float *T, int *n_P, int *n_T);

int main()
{
    int i, j, idx, val;
    int speed;
    int n_S, n_P, n_T;
    float max;
    fann_type *calc_out;
    fann_type input[3];
    struct fann *ann;
   
    ann = fann_create_from_file("TEST.net");
	
    mraa_i2c_context accel, gyro;
    float a_res, g_res, threshold=1.7;
    float *ax, *ay, *az, *gx, *gy, *gz;
    float *P_i, *T_i, *S_i, *S_min;
    double *t, start_epoch, end_epoch;
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

    float a,b,c;

    t = (double *) malloc(sizeof(double) * SAMPLES);
	
    ax = (float *) malloc(sizeof(float) * SAMPLES);
    ay = (float *) malloc(sizeof(float) * SAMPLES);
    ax = (float *) malloc(sizeof(float) * SAMPLES);

    gx = (float *) malloc(sizeof(float) * SAMPLES);
    gy = (float *) malloc(sizeof(float) * SAMPLES);
    gz = (float *) malloc(sizeof(float) * SAMPLES);
    
    while (1) {
        //sscanf(line,"%f %f %f\n",&a,&b,&c);
        
	for (i = 0; i < SAMPLES; i++)
	{
		gettimeofday(&start, NULL);
		accel_data = read_accel(accel, a_res);
		gyro_data = read_gyro(gyro, g_res);
		gettimeofday(&end, NULL);

	
		ax[i] = accel_data.x;
		ay[i] = accel_data.y;
		az[i] = accel_data.z;

		gx[i] = gyro_data.x;
		gy[i] = gyro_data.y;
		gz[i] = gyro_data.z;
	
		start_epoch = start.tv_sec - start.tv_usec/MILLION;
		end_epoch = end.tv_sec - end.tv_usec/MILLION;
		
		t[i] = (start_epoch + end_epoch)/2.0;
	}
	
	val = find_peaks_and_troughs(ax, SAMPLES, threshold, P_i, T_i, n_P, n_T);
	
	if (val < 0) {
		fprintf(stderr, "find_peaks_and_throughs failed\n");
		exit(EXIT_FAILURE);
	}

	//extract strides
		
	S_i = (float *) malloc(sizeof(float) * n_P); // P
	S_min = (float *) malloc(sizeof(float) * n_T); // T

	n_S = detect_strides(ax, S_i, S_min, P_i, T_i, t, n_P, n_T);


	//array of min, max, period for each stride (n_S)
	


	//determine walking speed for each stride
	for (j = 0; j < n_S; j++)		    
	max = -100;
       
        input[0] = (float) a;
        input[1] = (float) b;
        input[2] = (float) c;
        calc_out = fann_run(ann, input);

        for (i = 0; i < 4; i++) {
            if (calc_out[i] > max) {
                max = calc_out[i];
                speed = i+1;
            }
        }

	printf(" walking values: %f, %f, %f -> speed is %d\n", a, b,c, speed);
        sleep(1);
    }
    fann_destroy(ann);
    return 0;
}

/*
void collect_data (float *ax, float *ay, float *az, float *gx, float *gy, float *gz, double *t, mraa_i2c_context accel, mraa_i2c_context gyro, float a_res, float g_res)
{
	int k;
	struct timeval start, end;
	double start_epoch, end_epoch;
	data_t accel_data, gyro_data;
	for (k = 0; k < SAMPLES; k++)
	{
		gettimeofday(&start, NULL);
		accel_data = read_accel(accel, a_res);
		gyro_data = read_gyro(gyro, g_res);
		gettimeofday(&end, NULL);

	
		ax[k] = accel_data->x;
		ay[k] = accel_data.y;
		az[k] = accel_data.z;

		gx[k] = gyro_data.x;
		gy[k] = gyro_data.y;
		gz[k] = gyro_data.z;
	
		start_epoch = start.tv_sec - start.tv_usec/MILLION;
		end_epoch = end.tv_sec - end.tv_usec/MILLION;
		
		t[k] = (start_epoch + end_epoch)/2.0;
	}	
	return 0;
}
*/
int find_peaks_and_troughs(float *arr,int n_samples, float E, float *P, float *T, int *n_P, int *n_T)
{
	int a, b, i, d, _n_P, _n_T;

	i = -1; d = 0; a = 0; b = 0;
	_n_P = 0; _n_T = 0;

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

	
	/* DO NOT MODIFY ANYTHING AFTER THIS LINE */

	printf("P_i,P_t,P_v,T_i,T_t,T_v\n");            
	for (i = 0; i < n_P || i < n_T; i++) {
		/* Only peak data if there is peak data to write */
		if (i < n_P) {
			idx = (int) P_i[i];
			printf("%d,%20.10lf,%lf,",
					idx,
					t[idx],
					arr[idx]               //x
			       );
		} else {
			printf(",,,");
		}
		/* Only trough data if there is trough data to write */
		if (i < n_T) {
			idx = (int) T_i[i];
			printf("%d,%20.10lf,%lf\n",
					idx,
					t[idx],
					arr[idx]                //x
			       );
		} else {
			printf(",,\n");
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

