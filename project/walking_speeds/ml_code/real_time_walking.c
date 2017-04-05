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
    int i;
    int speed;
   
    float max;
    fann_type *calc_out;
    fann_type input[3];
    struct fann *ann;
   
    
    ann = fann_create_from_file("TEST.net");
	

    mraa_i2c_context accel, gyro;
    float ax*, ay*, az*, gx*, gy*, gz*, a_res, g_res;
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

    while (1) {
        //sscanf(line,"%f %f %f\n",&a,&b,&c);
        
	for (i = 0; i < 50; i++)
	{
		collect_data(ax[i], ay[i], az[i], gx[i], gy[i], gz[i], start_epoch[i], end_epoch[i]);

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
