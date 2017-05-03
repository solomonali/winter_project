#ifndef WALKING_H_
#define WALKING_H_

#include <unistd.h>
#include <stdio.h>
#include "floatfann.h"
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <signal.h>

int MILLION;
int SAMPLES;

void extract_walking(float *P_i, float *T_i, float *S_i, float *S_min, int n_S, int n_P, int n_T, float *periods, float *min, float *max);

#endif // WALKING_H_
