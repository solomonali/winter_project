#include <unistd.h>
#include <stdio.h>
#include "floatfann.h"
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <signal.h>

#define MILLION 1000000.0
#define SAMPLES 1500

sig_atomic_t volatile run_flag = 1;

void do_when_interrupted(int sig)
{


	if (sig == SIGINT)
		run_flag = 0;
}

int find_peaks_and_troughs(float *arr,int n_samples, float E, float *P, float *T, int *n_P, int *n_T);
void clear_buffer(float *arr, float val, int n);
void read_data(FILE *fp, double *t, float *ax, float *ay, float *az, float *gx, float *gy, float *gz, int samples);

int main()
{

    int i, j, idx, idx_min, idx_next, val, rv, sigma = 0, filenum = 0;
    int speed[100];
    int n_S, n_P, n_T;
    fann_type *calc_out;
    fann_type input[3];
    struct fann *ann;
   
    ann = fann_create_from_file("TEST.net");
	
    float min[1000], max[1000], fmax, threshold=1.7;
    float ax[SAMPLES+sigma], ay[SAMPLES+sigma], az[SAMPLES+sigma], gx[SAMPLES+sigma], gy[SAMPLES+sigma], gz[SAMPLES+sigma];
    float P_i[1000], T_i[1000], S_i[1000], S_min[1000];
    double t[SAMPLES+sigma], start_epoch, end_epoch, periods[1000], period;
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
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    char filename[50];

    while (1){
	fp = NULL;

	sprintf(filename, "data_%d.txt", filenum);
	
	while (fp == NULL)
	{
		fp = fopen(filename, "r");
	}

	i = 0;
//	printf("Reading file...\n");

	while ((read = getline(&line, &len, fp)) != -1)
	{
		rv = sscanf(line, "%lf,%f,%f,%f,%f,%f,%f\n", &t[i], &ax[i], &ay[i], &az[i], &gx[i], &gy[i], &gz[i]);
		i++;
	}

	fclose(fp);
	filenum++;
/*
	printf("File read...\n");

	printf("Checking data...\n");
	for (j = 0; j < SAMPLES; j++)
	{
		printf("i: %d\t t: %lf\t ax: %f\t ay: %f\t az: %f\t gx: %f\t gy: %f\t gz: %f\n", j, t[j], ax[j], ay[j], az[j], gx[j], gy[j], gz[j]);
	}

	printf("Finding peaks and troughs...\n");
 */
	val = find_peaks_and_troughs(ax, SAMPLES, threshold, P_i, T_i, &n_P, &n_T);
//	printf("Check...\n");

	if (val < 0) {
		fprintf(stderr, "find_peaks_and_throughs failed\n");
		exit(EXIT_FAILURE);
	}

//	printf("Detecting strides...\n");

	n_S = detect_strides(S_i, S_min, P_i, T_i, t, n_P, n_T);
	

	if (n_S == 0)
	{
		printf("No strides detected...\n");
	}
	else
	{
//		printf("Extracting features...\n");
		for (i = 0; i < n_S; i++) 
		{
			idx = (int) S_i[i];
			idx_min = (int) S_min[i];
			idx_next = (int) S_i[i+1];
			((i+1)!=n_S)? period = t[idx_next]- t[idx]: period;
			periods[i] = period/10.0;
			max[i] = ax[idx]/10.0;
			min[i] = ax[idx_min]/10.0;
		}

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
		}
	}
    }
    fann_destroy(ann);
    return 0;
}

int find_peaks_and_troughs(float *arr,int n_samples, float E, float *P, float *T, int *n_P, int *n_T)
{
	int a, b, i, d, _n_P, _n_T;

	d = 0; a = 0; b = 0;
	_n_P = 0; _n_T = 0;

	clear_buffer(P, 0.0f, 1000);
	clear_buffer(T, 0.0f, 1000);

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
	
	clear_buffer(S_i, 0.0f, 1000);
	clear_buffer(S_min, 0.0f, 1000);

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

void read_data(FILE *fp, double *t, float *ax, float *ay, float *az, float *gx, float *gy, float *gz, int samples)
{
	int i, r;
	size_t len;
	ssize_t read;
	char *line = NULL;
	while(read = getline(&line, &len, fp) != -1)
	{
//		printf("test1\n");
		r = sscanf(line, "%lf,%f,%f,%f,%f,%f,%f\n", &t[i], &ax[i], &ay[i], &az[i], &gx[i], &gy[i], &gz[i]);
//		printf("test2\n");
		/*if (r == EOF)
		{
			fprintf(stderr, "Error reading file...\n");
			exit(EXIT_FAILURE);
		}*/

	}
	
	return;	
}
