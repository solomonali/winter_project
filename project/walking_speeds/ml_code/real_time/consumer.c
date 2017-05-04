#include <unistd.h>
#include <stdio.h>
#include "floatfann.h"
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/file.h>

#define MILLION 1000000.0
#define SAMPLES 1000

sig_atomic_t volatile run_flag = 1;

void do_when_interrupted(int sig)
{


	if (sig == SIGINT)
		run_flag = 0;
}

int find_peaks_and_troughs(float *arr,int n_samples, float E, float *P, float *T, int *n_P, int *n_T)
{
	int a, b, i, d, _n_P, _n_T;

	d = 0; a = 0; b = 0;
	_n_P = 0; _n_T = 0;

	i = -1;
	
	while (i != n_samples) {
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
		} else {
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
	
	//printf("Returning values...\n");
	(*n_P) = _n_P;
	(*n_T) = _n_T;
	return 0;
}

int detect_strides(float *S_i, float *S_min, float *P_i, float *T_i, double *t, int n_P, int n_T)
{
	int a, b, c, i, idx, n_S;
	a = 0; b = 1; n_S = 0; c = 0; 
	
	while(b < n_P)                     //P
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

void walk_features(float *ax, double *t, float *S_i, float *S_min, int n_S, int n_P, int n_T, double *periods, float *min, float *max)
{
	int i, idx, idx_next, idx_min;
	double period;

	for (i = 0; i < n_S; i++)
	{
		idx = (int) S_i[i];
		idx_min = (int) S_min[i];
		idx_next = (int) S_i[i+1];
		//((i+1)!=n_S)? period = t[idx_next]- t[idx]: period;
		period = t[idx_next] - t[idx];
		periods[i] = period/10.0;
		max[i] = ax[idx]/10.0;
		min[i] = ax[idx_min]/10.0;
//		printf("Period: %f\n", periods[i]);
//		printf("Max: %f\n", max[i]);
//		printf("Min: %f\n", min[i]);

	}

	return;
}

int main()
{

    int i, j, idx, idx_min, idx_next, val, rv, sigma = 0, filenum = 0;
    int size = 150;
    int speed[size];
    int n_S, n_P, n_T;
    fann_type *calc_out;
    fann_type input[3];
    struct fann *ann;
   
    ann = fann_create_from_file("TEST.net");
	
    float min[size], max[size], fmax, threshold=1.7;
    float ax[SAMPLES+sigma], ay[SAMPLES+sigma], az[SAMPLES+sigma], gx[SAMPLES+sigma], gy[SAMPLES+sigma], gz[SAMPLES+sigma];
    float P_i[size*5], T_i[size*5], S_i[size], S_min[size];
    double t[SAMPLES+sigma], start_epoch, end_epoch, periods[size];
    struct timeval start, end;

    signal(SIGINT, do_when_interrupted);
    
    //P_i = (float *) malloc+(sizeof(float) * (100));
    //T_i = (float *) malloc(sizeof(float) * (100));
    //periods = (double *) malloc(sizeof(double) * 100);
    //min = (float *) malloc(sizeof(float) * 100);
    //max = (float *) malloc(sizeof(float) * 100);
    //S_i = (float *) malloc(sizeof(float) * 100); // P
    //S_min = (float *) malloc(sizeof(float) * 100); // T
    //speed = (int *) malloc(sizeof(int) * 100);

    FILE *fp;
    int fd;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    char filename[50];

    for (i = 0; i < size; i++)
    {
	    periods[i] = 0.0;
	    speed[i] = 0;
    }

    sleep(2);

    while (1){
	clear_buffer(P_i, 0.0f, size*5);
	clear_buffer(T_i, 0.0f, size*5);
	clear_buffer(min, 0.0f, size);
	clear_buffer(max, 0.0f, size);
	clear_buffer(S_i, 0.0f, size);
	clear_buffer(S_min, 0.0f, size);

	fp = NULL;

	sprintf(filename, "data_%d.txt", filenum);
	
	while (fp == NULL)
	{
		fp = fopen(filename, "r");
	}

	fd = fileno(fp);
	flock(fd, LOCK_EX);	

	i = 0;
//	printf("Reading file...\n");

	while ((read = getline(&line, &len, fp)) != -1)
	{
		rv = sscanf(line, "%lf,%f,%f,%f,%f,%f,%f\n", &t[i], &ax[i], &ay[i], &az[i], &gx[i], &gy[i], &gz[i]);
		i++;
	}

	fclose(fp);
	filenum++;
	//printf("File read...\n");

	//printf("Checking data...\n");

//	printf("Finding peaks and troughs...\n");
	val = find_peaks_and_troughs(ax, SAMPLES, threshold, P_i, T_i, &n_P, &n_T);
//	printf("Check...\n");

	if (val < 0) {
		fprintf(stderr, "find_peaks_and_throughs failed\n");
		exit(EXIT_FAILURE);
	}

//	printf("Detecting strides...\n");

//	printf("# of Peaks: %d\t # of Troughs: %d\n", n_P, n_T);

	n_S = detect_strides(S_i, S_min, P_i, T_i, t, n_P, n_T);
	

	if (n_S == 0)
	{
		printf("No strides detected...\n");
		printf("Next set...\n");
	}
	else
	{
//		printf("Extracting features...\n");
		
		walk_features(ax, t, S_i, S_min, n_S, n_P, n_T, periods, min, max);

		//determine walking speed for each stride
//		printf("Determining walking speeds...\n");
		for (j = 0; j < n_S; j++)
		{
			fmax = -100;   
		        input[0] = (float) periods[j];
        		input[1] = (float) max[j];
	        	input[2] = (float) min[j];
	        	calc_out = fann_run(ann, input);
	
		        for (i = 0; i < 4; i++) 
			{
	         	   if (calc_out[i] > fmax) 
			   {
                		fmax = calc_out[i];
	                	speed[j] = i+1;
	        	    }
       			}
		}

		for (i = 0; i < n_S; i++)
		{
			printf("Period: %f\t Max: %f\t Min: %f\t -> speed is %d\n", periods[i], max[i], min[i], speed[i]);
			speed[i] = 0;
			periods[i] = 0.0;
		}
		printf("Next set...\n");
	}
    }
    fann_destroy(ann);
    return 0;
}
