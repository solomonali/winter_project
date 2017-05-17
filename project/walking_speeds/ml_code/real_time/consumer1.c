#include <unistd.h>
#include <stdio.h>
#include "floatfann.h"
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/file.h>
#include <math.h>

#define MILLION 1000000.0
//change SAMPLES for BOTH PRODUCER AND CONSUMER
#define SAMPLES 4000

/*
 * Caculates mean of first <n> samples in <*arr>
 */
float calculate_mean(float *arr, int n)
{
	float total;
	int i;

	total = 0.0f;
	for (i = 0; i < n; i++) {
		total += arr[i];
	}

	return total/((float) n);
}
float find_sum(float *arr, int n1, int n2){
	float sum=0.0f;
	int i;
	for (i=n1; i<n2; i++){
		sum=sum+arr[i];
	}
	return sum;
}
float var(float *arr, int n1, int n2, float mean){
	float var, sum=0.0f;
	int i;
	for(i=n1;i<n2;i++){
		sum = sum+(arr[i]-mean)*(arr[i]-mean);
	}
		var=sum/(n2-n1);
		return var;
}

float find_mean(float *arr, int n1, int n2){
	float sum=0.0f;
	int i;
	for(i=n1;i<n2;i++){
		sum=sum+arr[i];
	}
	return sum/(n2-n1);
}

int find_peaks_and_troughs(float *arr,int n_samples, float E, int *P, int *T, int *n_P, int *n_T)
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

int detect_strides(int *S_i, int *S_min, int *P_i, int *T_i, int n_P, int n_T)
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

void LPF(float *arr, float *smooth, int samples, int k)
{
	int i;
	smooth[0] = arr[0];
	for (i = 1; i < samples; i++)
	{
		smooth[i] = (1-2*exp(-k)) * smooth[i-1] + arr[i];
	}

	for (i = 0; i < samples; i++)
	{smooth[i] = smooth[i]/15.0;}

	return;
}

/*
void split_strides(int *S_i, int indexes[n_S][splits])
{
	int i, j, diff;
	for(i = 0; i < n_S; i++)
	{
		diff = (S_i[i+1] - S_i[i])/splits;
		for(j = 0; j < splits; j++)
		{
			indexes[i][j] = S_i[i] + j*diff;
		}
			
	}

	return;
}
*/

void clear_buffer(float *arr, float val, int n)
{
	int i;
	for (i = 0; i < n; i++)
		arr[i] = val;
}

void walk_features(float *arr, double *t, int *S_i, int *S_min, int n_S, int n_P, int n_T, double *periods, float *min, float *max, int *idx, int *idx_min, int *idx_next)
{
	int i;
	double period;

	for (i = 0; i < n_S; i++)
	{
		idx[i] = S_i[i];
		idx_min[i] = S_min[i];
		idx_next[i] = S_i[i+1];
		period = (t[idx_next[i]] - t[idx[i]]);
		if (period < 0) { period = -period; }
		periods[i] = period/10.0;
		max[i] = arr[idx[i]]/1000.0;
		min[i] = arr[idx_min[i]]/1000.0;
	}

	return;
}


//add feauture extraction algorithms here
//add parameters and change the return type if necessary
void run_features()
{}

void calc_stats(float *x_ac, float *y_ac, float *z_ac, float *x_gy, float *y_gy, float *z_gy, int n_S, int *idx_next, int *idx, float *x_ac_mean1, float *x_ac_mean2, float *x_ac_mean3,float *x_ac_mean4, float *x_ac_var1, float *x_ac_var2, float *x_ac_var3, float *x_ac_var4, float *y_ac_mean1, float *y_ac_mean2, float *y_ac_mean3,float *y_ac_mean4, float *y_ac_var1, float *y_ac_var2, float *y_ac_var3, float *y_ac_var4, float *z_ac_mean1, float *z_ac_mean2, float *z_ac_mean3,float *z_ac_mean4, float *z_ac_var1, float *z_ac_var2, float *z_ac_var3, float *z_ac_var4, float *x_gy_mean1, float *x_gy_mean2, float *x_gy_mean3,float *x_gy_mean4, float *x_gy_var1, float *x_gy_var2, float *x_gy_var3, float *x_gy_var4, float *y_gy_mean1, float *y_gy_mean2, float *y_gy_mean3,float *y_gy_mean4, float *y_gy_var1, float *y_gy_var2, float *y_gy_var3, float *y_gy_var4, float *z_gy_mean1, float *z_gy_mean2, float *z_gy_mean3,float *z_gy_mean4, float *z_gy_var1, float *z_gy_var2, float *z_gy_var3, float *z_gy_var4) 
{
  int i, seg;
  for (i = 0; i < n_S; i++)
  {
  
  seg = (idx_next[i] - idx[i])/4;
  
  x_ac_mean1[i]=find_mean(x_ac, idx[i], idx[i]+seg);
  x_ac_mean2[i]=find_mean(x_ac, idx[i]+seg, idx[i]+2*seg);
  x_ac_mean3[i]=find_mean(x_ac, idx[i]+2*seg,idx[i]+3*seg);
  x_ac_mean4[i]=find_mean(x_ac, idx[i]+3*seg, idx_next[i]);
  
  y_ac_mean1[i]=find_mean(y_ac, idx[i], idx[i]+seg);
  y_ac_mean2[i]=find_mean(y_ac, idx[i]+seg, idx[i]+2*seg);
  y_ac_mean3[i]=find_mean(y_ac, idx[i]+2*seg, idx[i]+3*seg);
  y_ac_mean4[i]=find_mean(y_ac, idx[i]+3*seg, idx_next[i]);
    
  z_ac_mean1[i]=find_mean(z_ac, idx[i], idx[i]+seg);
  z_ac_mean2[i]=find_mean(z_ac, idx[i]+seg, idx[i]+2*seg);
  z_ac_mean3[i]=find_mean(z_ac, idx[i]+2*seg, idx[i]+3*seg);
  z_ac_mean4[i]=find_mean(z_ac, idx[i]+3*seg, idx_next[i]);
  
  x_ac_var1[i]=var(x_ac, idx[i], idx[i]+seg, x_ac_mean1[i]);
  x_ac_var2[i]=var(x_ac, idx[i]+seg, idx[i]+2*seg, x_ac_mean2[i]);
  x_ac_var3[i]=var(x_ac, idx[i]+2*seg, idx[i]+3*seg, x_ac_mean3[i]);
  x_ac_var4[i]=var(x_ac, idx[i]+3*seg, idx_next[i], x_ac_mean4[i]);
  
  y_ac_var1[i]=var(y_ac, idx[i], idx[i]+seg, y_ac_mean1[i]);
  y_ac_var2[i]=var(y_ac, idx[i]+seg, idx[i]+2*seg, y_ac_mean2[i]);
  y_ac_var3[i]=var(y_ac, idx[i]+2*seg, idx[i]+3*seg, y_ac_mean3[i]);
  y_ac_var4[i]=var(y_ac, idx[i]+3*seg, idx_next[i], y_ac_mean4[i]);
  
  z_ac_var1[i]=var(z_ac, idx[i], idx[i]+seg, z_ac_mean1[i]);
  z_ac_var2[i]=var(z_ac, idx[i]+seg, idx[i]+2*seg, z_ac_mean2[i]);
  z_ac_var3[i]=var(z_ac, idx[i]+2*seg, idx[i]+3*seg, z_ac_mean3[i]);
  z_ac_var4[i]=var(z_ac, idx[i]+3*seg, idx_next[i], z_ac_mean4[i]);
  
  x_gy_mean1[i]=find_mean(x_gy, idx[i], idx[i]+seg);
  x_gy_mean2[i]=find_mean(x_gy, idx[i]+seg, idx[i]+2*seg);
  x_gy_mean3[i]=find_mean(x_gy, idx[i]+2*seg,idx[i]+3*seg);
  x_gy_mean4[i]=find_mean(x_gy, idx[i]+3*seg, idx_next[i]);
  
  y_gy_mean1[i]=find_mean(y_gy, idx[i], idx[i]+seg);
  y_gy_mean2[i]=find_mean(y_gy, idx[i]+seg, idx[i]+2*seg);
  y_gy_mean3[i]=find_mean(y_gy, idx[i]+2*seg, idx[i]+3*seg);
  y_gy_mean4[i]=find_mean(y_gy, idx[i]+3*seg, idx_next[i]);
    
  z_gy_mean1[i]=find_mean(z_gy, idx[i], idx[i]+seg);
  z_gy_mean2[i]=find_mean(z_gy, idx[i]+seg, idx[i]+2*seg);
  z_gy_mean3[i]=find_mean(z_gy, idx[i]+2*seg, idx[i]+3*seg);
  z_gy_mean4[i]=find_mean(z_gy, idx[i]+3*seg, idx_next[i]);

  x_gy_var1[i]=var(x_gy, idx[i], idx[i]+seg, x_gy_mean1[i]);
  x_gy_var2[i]=var(x_gy, idx[i]+seg, idx[i]+2*seg, x_gy_mean2[i]);
  x_gy_var3[i]=var(x_gy, idx[i]+2*seg, idx[i]+3*seg, x_gy_mean3[i]);
  x_gy_var4[i]=var(x_gy, idx[i]+3*seg, idx_next[i], x_gy_mean4[i]);
  
  y_gy_var1[i]=var(y_gy, idx[i], idx[i]+seg, y_gy_mean1[i]);
  y_gy_var2[i]=var(y_gy, idx[i]+seg, idx[i]+2*seg, y_gy_mean2[i]);
  y_gy_var3[i]=var(y_gy, idx[i]+2*seg, idx[i]+3*seg, y_gy_mean3[i]);
  y_gy_var4[i]=var(y_gy, idx[i]+3*seg, idx_next[i], y_gy_mean4[i]);
  
  z_gy_var1[i]=var(z_gy, idx[i], idx[i]+seg, z_gy_mean1[i]);
  z_gy_var2[i]=var(z_gy, idx[i]+seg, idx[i]+2*seg, z_gy_mean2[i]);
  z_gy_var3[i]=var(z_gy, idx[i]+2*seg, idx[i]+3*seg, z_gy_mean3[i]);
  z_gy_var4[i]=var(z_gy, idx[i]+3*seg, idx_next[i], z_gy_mean4[i]);
  

  }

  return;
  
}

void ascent_features()
{}
void descent_features()
{}

int main()
{

    int i, j, val, rv, sigma = 0, filenum = 0;
    int index[SAMPLES+sigma];
    int size = 400, splits = 4, k_LPF = 4;
    int speed[size], turn[size];
    const char *speeds[3], *turns[3];
    speeds[0] = "FAST";
    speeds[1] = "MEDIUM";
    speeds[2] = "SLOW";
    
    turns[0] = "NO TURN";
    turns[1] = "LEFT TURN";
    turns[2] = "RIGHT TURN";
    
    int n_S, n_P, n_T;
    fann_type *walk_calc_out, *turn_calc_out;
    fann_type walk_input[3], turn_input[16];
    //intialize new fann networks here for different motions
    struct fann *f_walk, *f_turn;
   
    //create new fann with appropriate train file
    f_walk = fann_create_from_file("walk.net");
    f_turn = fann_create_from_file("turn.net");
	
    float threshold_ax, threshold_ay, threshold_az, threshold_gx, threshold_gy, threshold_gz = 200; 
    float min[size], max[size], fmax;
    float ax[SAMPLES+sigma], ay[SAMPLES+sigma], az[SAMPLES+sigma], gx[SAMPLES+sigma], gy[SAMPLES+sigma], gz[SAMPLES+sigma];
    float ax_s[SAMPLES+sigma], ay_s[SAMPLES+sigma], az_s[SAMPLES+sigma], gx_s[SAMPLES+sigma], gy_s[SAMPLES+sigma], gz_s[SAMPLES+sigma];
    float  x_ac_mean1[size],x_ac_mean2[size],x_ac_mean3[size],x_ac_mean4[size], x_ac_var1[size],x_ac_var2[size],x_ac_var3[size],x_ac_var4[size];
    float  x_gy_mean1[size],x_gy_mean2[size],x_gy_mean3[size],x_gy_mean4[size], x_gy_var1[size],x_gy_var2[size],x_gy_var3[size],x_gy_var4[size];
    float  y_ac_mean1[size],y_ac_mean2[size],y_ac_mean3[size],y_ac_mean4[size], y_ac_var1[size],y_ac_var2[size],y_ac_var3[size],y_ac_var4[size];
    float  y_gy_mean1[size],y_gy_mean2[size],y_gy_mean3[size],y_gy_mean4[size], y_gy_var1[size],y_gy_var2[size],y_gy_var3[size],y_gy_var4[size];
    float  z_ac_mean1[size],z_ac_mean2[size],z_ac_mean3[size],z_ac_mean4[size], z_ac_var1[size],z_ac_var2[size],z_ac_var3[size],z_ac_var4[size];
    float  z_gy_mean1[size],z_gy_mean2[size],z_gy_mean3[size],z_gy_mean4[size], z_gy_var1[size],z_gy_var2[size],z_gy_var3[size],z_gy_var4[size];        
    
    int P_i[size*5], T_i[size*5], S_i[size], S_min[size];
    int idx[size], idx_min[size], idx_next[size];
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
		P_i[i] = 0;
		T_i[i] = 0;
	}

	for (i = 0; i < size; i++)
	{
		min[i] = 0.0f;
		S_i[i] = 0;
		S_min[i] = 0;
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

	while ((read = getline(&line, &len, fp)) != -1 && i < SAMPLES)
	{
		rv = sscanf(line, "%d,%lf,%f,%f,%f,%f,%f,%f\n", &index[i], &t[i], &ax[i], &ay[i], &az[i], &gx[i], &gy[i], &gz[i]);
		i++;
	}

	fclose(fp);
	filenum++;
	
	/*
	for (i = 0; i < SAMPLES; i++)
	{
		printf("%d\n", index[i]);
	}
	*/

	//find peaks and throughs for different axes w different thresholds
	
	LPF(gz, gz, SAMPLES, k_LPF);

	val = find_peaks_and_troughs(gz, SAMPLES, threshold_gz, P_i, T_i, &n_P, &n_T);
	
	for (i = 0; i < n_P; i++)
	{printf("Peak: %d\n", P_i[i]);}

	for (i = 0; i < n_T; i++)
	{printf("Trough: %d\n", T_i[i]);}
	if (val < 0) {
		fprintf(stderr, "find_peaks_and_throughs failed\n");
		exit(EXIT_FAILURE);
	}

	n_S = detect_strides(S_i, S_min, P_i, T_i, n_P, n_T);	

	if (n_S < 1)
	{
		printf("No strides detected...\n");
		printf("Next set...\n");
	}
	else
	{
		for (i = 0; i < n_S; i++)
		{
		//	printf("x_ac_var2: %f\tz_gy_mean3: %f\ty_ac_mean1: %f\n", x_ac_var2[i], z_gy_mean3[i], y_ac_mean1[i]); 
			printf("Stride: %d\n", S_i[i]);
		}

		//extract motion features here
		printf("Finding features...\n");

		walk_features(gz_s, t, S_i, S_min, n_S, n_P, n_T, periods, min, max, idx, idx_min, idx_next);

		printf("Walk features found...\n");

		calc_stats( ax,  ay,  az,  gx,  gy,  gz, n_S, idx_next, idx,  x_ac_mean1,  x_ac_mean2,  x_ac_mean3, x_ac_mean4,  x_ac_var1,  x_ac_var2,  x_ac_var3,  x_ac_var4,  y_ac_mean1,  y_ac_mean2,  y_ac_mean3, y_ac_mean4,  y_ac_var1,  y_ac_var2,  y_ac_var3,  y_ac_var4,  z_ac_mean1,  z_ac_mean2,  z_ac_mean3, z_ac_mean4,  z_ac_var1,  z_ac_var2,  z_ac_var3,  z_ac_var4,  x_gy_mean1,  x_gy_mean2,  x_gy_mean3, x_gy_mean4,  x_gy_var1,  x_gy_var2,  x_gy_var3,  x_gy_var4,  y_gy_mean1,  y_gy_mean2,  y_gy_mean3, y_gy_mean4,  y_gy_var1,  y_gy_var2,  y_gy_var3,  y_gy_var4,  z_gy_mean1,  z_gy_mean2,  z_gy_mean3, z_gy_mean4,  z_gy_var1,  z_gy_var2,  z_gy_var3,  z_gy_var4);
   

		printf("Turn Features found...\n");

			//pass feautres to fann
		for (j = 0; j < n_S; j++)
		{
			fmax = -100;   
		        walk_input[0] = (float) periods[j];
        		walk_input[1] = (float) max[j];
	        	walk_input[2] = (float) min[j];
	        	walk_calc_out = fann_run(f_walk, walk_input);
		        for (i = 0; i < 3; i++) 
			{
	       	 	   if (walk_calc_out[i] > fmax) 
			   {
               			fmax = walk_calc_out[i];
	                	speed[j] = i;
	        	   }
			}
		}

		for (j = 0; j < n_S; j++)
		{
			fmax = -100;
      printf("%f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f\n", x_gy_mean1[j]/100, x_gy_mean2[j]/10, x_gy_mean3[j]/10, x_gy_mean4[j]/10, z_ac_var1[j], z_ac_var2[j], z_ac_var3[j], z_ac_var4[j], y_gy_var1[j]/1000, y_gy_var2[j]/100, y_gy_var3[j]/100, y_gy_var4[j]/100, z_gy_var1[j]/10000, z_gy_var2[j]/10000, z_gy_var3[j]/10000, z_gy_var4[j]/10000);
			turn_input[0] = (float) x_gy_mean1[j]/100;
			turn_input[1] = (float) x_gy_mean2[j]/10;
			turn_input[2] = (float) x_gy_mean3[j]/10;
			turn_input[3] = (float) x_gy_mean4[j]/10;
			turn_input[4] = (float) z_ac_var1[j];
			turn_input[5] = (float) z_ac_var2[j];
			turn_input[6] = (float) z_ac_var3[j];
			turn_input[7] = (float) z_ac_var4[j];
			turn_input[8] = (float) y_gy_var1[j]/1000;
			turn_input[9] = (float) y_gy_var2[j]/100;
			turn_input[10] = (float) y_gy_var3[j]/100;
			turn_input[11] = (float) y_gy_var4[j]/100;
			turn_input[12] = (float) z_gy_var1[j]/10000;
			turn_input[13] = (float) z_gy_var2[j]/10000;
			turn_input[14] = (float) z_gy_var3[j]/10000;
			turn_input[15] = (float) z_gy_var4[j]/10000;
			
			
			turn_calc_out = fann_run(f_turn, turn_input);
			for (i = 0; i < 3; i++)
			{
				if(turn_calc_out[i] > fmax)
				{
					fmax = turn_calc_out[i];
					turn[j] = i;
				}
			}	
		}	

		printf("FANN complete...\n");


		//print output
		for (i = 0; i < n_S; i++)
		{
			printf("Speed is %s\n", speeds[speed[i]]);
			printf("Turning: %s\n", turns[turn[i]]);
			speed[i] = 0;
			periods[i] = 0.0;
		}
		printf("Next set...\n");
	}
    }
    
    fann_destroy(f_walk);
    fann_destroy(f_turn);

    return 0;
}
