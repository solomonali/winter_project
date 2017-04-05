#include <unistd.h>

#include <stdio.h>
#include "floatfann.h"

#include <string.h>
#include <stdlib.h>

#include <sys/time.h>
#include <signal.h>
#include "LSM9DSO.h"
#include <mraa/i2c.h>

int main()
{
    int i, val;
    int speed;
    int n_S, n_P, n_T, nSamples = 2000;
    float max;
    fann_type *calc_out;
    fann_type input[3];
    struct fann *ann;
   
    
    ann = fann_create_from_file("TEST.net");
	

    mraa_i2c_context accel, gyro;
    float a_res, g_res, threshold=1.7;
    float ax*, ay*, az*, gx*, gy*, gz*;
    float P_i*, T_i*, S_i*;
    double start_epoch*, end_epoch*;
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
	
    ax = (double *) malloc(sizeof(double) * nSamples);
    ay = (double *) malloc(sizeof(double) * nSamples);
    ax = (double *) malloc(sizeof(double) * nSamples);

    gx = (double *) malloc(sizeof(double) * nSamples);
    gy = (double *) malloc(sizeof(double) * nSamples);
    gz = (double *) malloc(sizeof(double) * nSamples);
    
    while (1) {
        //sscanf(line,"%f %f %f\n",&a,&b,&c);
        
	for (i = 0; i < nSamples; i++)
	{
		collect_data(ax[i], ay[i], az[i], gx[i], gy[i], gz[i], start_epoch[i], end_epoch[i]);
	}
	
	val = find_peaks_and_troughs(ax, nSamples, threshold, P_i, T_i, &n_p, &n_T);
	
	if (val < 0) {
		fprintf(stderr, "find_peaks_and_throughs failed\n");
		exit(EXIT_FAILURE);
	}

		


	    
	    
	    
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
        //read = getline(&line, &len, fp); //to discard every other line
    }
    //fclose(fp);
    fann_destroy(ann);
    return 0;
}


void collect_data (float &ax, float &ay, float &az, float &gx, float &gy, float &gz, double &start_epoch, double &end_epoch)
{
	struct timeval start, end;
	gettimeofday(&start, NULL);
	accel_data = read_accel(accel, a_res);
	gyro_data = read_gyro(gyro, g_res);
	gettimeofday(&end, NULL);

	
	ax = accel_data.x;
	ay = accel_data.y;
	az = accel_data.z;

	gx = gyro_data.x;
	gy = gyro_data.y;
	gz = gyro_data.z;
	
	start_epoch = start.tv_sec - start.tv_usec/MILLION;
	end_epoch = end.tv_sec - end.tv_usec/MILLION;

	return 0;
}

int 
find_peaks_and_troughs(
		float *arr, 	// signal 
		int n_samples, 	// number of samples present in the signal
		float E, 	// threshold for peak detection
		// arrays that will store the indicies of the located
		// peaks and troughs
		float *P, float *T,
		// number of peaks (n_P) and number of troughs (n_T)
		// found in the data set *arr
		int *n_P, int *n_T
		)
{
	int a, b, i, d, _n_P, _n_T;

	i = -1; d = 0; a = 0; b = 0;
	_n_P = 0; _n_T = 0;

	clear_buffer(P, 0.0f, n_samples);
	clear_buffer(T, 0.0f, n_samples);

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

void detect_strides( )
{
	S_i = (float *) malloc(sizeof(float) * n_P); // P
	float *S_min;
	S_min = (float *) malloc(sizeof(float) * n_T); //T    //for min (troughs)

  	int a , b , c;
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

	/* open the output file to write the peak and trough data */
	printf("Attempting to write to file \'%s\'.\n", ofile_pt_name);
	fp = fopen(ofile_pt_name, "w");
	if (fp == NULL) {
		fprintf(stderr, 
				"Failed to write to file \'%s\'.\n", 
				ofile_pt_name
		       );
		exit(EXIT_FAILURE);
	}

	fprintf(fp, "P_i,P_t,P_v,T_i,T_t,T_v\n");            
	for (i = 0; i < n_P || i < n_T; i++) {
		/* Only peak data if there is peak data to write */
		if (i < n_P) {
			idx = (int) P_i[i];
			fprintf(fp, "%d,%20.10lf,%lf,",
					idx,
					t[idx],
					x_ac[idx]               //x
			       );
		} else {
			fprintf(fp, ",,,");
		}
		/* Only trough data if there is trough data to write */
		if (i < n_T) {
			idx = (int) T_i[i];
			fprintf(fp, "%d,%20.10lf,%lf\n",
					idx,
					t[idx],
					x_ac[idx]                //x
			       );
		} else {
			fprintf(fp, ",,\n");
		}
	}
}


