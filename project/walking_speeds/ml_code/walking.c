#include "walking.h"
#include <unistd.h>
#include <stdio.h>
#include "floatfann.h"
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <signal.h>

float MILLION;
int SAMPLES;

void extract_walking (float *S_i, float *S_min, int n_S, int n_P, int n_T, float *periods, float *min, float *max)
{

    int i, idx, idx_min, idx_next;

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

    return;
}
