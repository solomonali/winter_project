#include <unistd.h>
#include <stdio.h>
#include "floatfann.h"
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/file.h>

#define MILLION 1000000.0
//change SAMPLES for BOTH PRODUCER AND CONSUMER
#define SAMPLES 4000

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

void walk_features(float *arr, double *t, float *S_i, float *S_min, int n_S, int n_P, int n_T, double *periods, float *min, float *max)
{
	int i, idx, idx_next, idx_min;
	double period;

	for (i = 0; i < n_S; i++)
	{
		idx = (int) S_i[i];
		idx_min = (int) S_min[i];
		idx_next = (int) S_i[i+1];
		period = (t[idx_next] - t[idx]);
		if (period < 0) { period = -period; }
		periods[i] = period/10.0;
		max[i] = arr[idx]/1000.0;
		min[i] = arr[idx_min]/1000.0;
	}

	return;
}


//add feauture extraction algorithms here
//add parameters and change the return type if necessary
void run_features()
{}
void turn_feautures() 
{}
void ascent_features()
{}
void descent_features()
{}

int main()
{

    int i, j, idx, idx_min, idx_next, val, rv, sigma = 0, filenum = 0;
    int size = 400;
    int speed[size];
    int n_S, n_P, n_T;
    fann_type *calc_out;
    fann_type input[3];
    //intialize new fann networks here for different motions
    struct fann *f_walk;
   
    //create new fann with appropriate train file
    f_walk = fann_create_from_file("walk.net");
	
    float threshold_ax, threshold_ay, threshold_az, threshold_gx, threshold_gy, threshold_gz = 200; 
    float min[size], max[size], fmax;
    float ax[SAMPLES+sigma], ay[SAMPLES+sigma], az[SAMPLES+sigma], gx[SAMPLES+sigma], gy[SAMPLES+sigma], gz[SAMPLES+sigma];
    float P_i[size*5], T_i[size*5], S_i[size], S_min[size];
    double t[SAMPLES+sigma], start_epoch, end_epoch, periods[size];
    struct timeval start, end;

    FILE *fp;
    int fd, linecount;
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
	
	for (i = 0; i < size*5; i++)
	{
		P_i[i] = 0.0f;
		T_i[i] = 0.0f;
	}

	for (i = 0; i < size; i++)
	{
		min[i] = 0.0f;
		max[i] = 0.0f;
		S_i[i] = 0.0f;
		S_min[i] = 0.0f;
	}

	fp = NULL;

	sprintf(filename, "data_%d.txt", filenum);
	
	linecount = 0;

	while (fp == NULL)
	{
		fp = fopen(filename, "r");
	}

	fd = fileno(fp);
	flock(fd, LOCK_EX);	

	i = 0;

	while ((read = getline(&line, &len, fp)) != -1 && i < 1500)
	{
		rv = sscanf(line, "%lf,%f,%f,%f,%f,%f,%f\n", &t[i], &ax[i], &ay[i], &az[i], &gx[i], &gy[i], &gz[i]);
		i++;
	}

	fclose(fp);
	filenum++;
	
	//find peaks and throughs for different axes w different thresholds
	val = find_peaks_and_troughs(gz, SAMPLES, threshold_gz, P_i, T_i, &n_P, &n_T);

	if (val < 0) {
		fprintf(stderr, "find_peaks_and_throughs failed\n");
		exit(EXIT_FAILURE);
	}

	n_S = detect_strides(S_i, S_min, P_i, T_i, t, n_P, n_T);	

	if (n_S < 1)
	{
		printf("No strides detected...\n");
		printf("Next set...\n");
	}
	else
	{
	
		//extract motion features here
		walk_features(gz, t, S_i, S_min, n_S, n_P, n_T, periods, min, max);


		//pass feautres to fann
		for (j = 0; j < n_S; j++)
		{
			fmax = -100;   
		        input[0] = (float) periods[j];
        		input[1] = (float) max[j];
	        	input[2] = (float) min[j];
	        	calc_out = fann_run(f_walk, input);
	
		        for (i = 0; i < 4; i++) 
			{
	         	   if (calc_out[i] > fmax) 
			   {
                		fmax = calc_out[i];
	                	speed[j] = i+1;
	        	    }
       			}
		}

		//print output
		for (i = 0; i < n_S; i++)
		{
			printf("Period: %f\t Max: %f\t Min: %f\t -> speed is %d\n", periods[i], max[i], min[i], speed[i]);
			speed[i] = 0;
			periods[i] = 0.0;
		}
		printf("Next set...\n");
	}
    }
    fann_destroy(f_walk);
    return 0;
}
