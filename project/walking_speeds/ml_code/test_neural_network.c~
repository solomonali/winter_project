#include <unistd.h>

#include <stdio.h>
#include "floatfann.h"

#include <string.h>
#include <stdlib.h>

int main()
{
    int i;
    int speed;
   
    float max;
    fann_type *calc_out;
    fann_type input[3];
    struct fann *ann;
   
    
    ann = fann_create_from_file("TEST.net");

    FILE *fp;
    char *file_name = "test.txt";
    float a,b,c;
    fp = fopen(file_name,"r");
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    
    read = getline(&line, &len, fp); //discard the frist line


    while ((read = getline(&line, &len, fp)) != -1) {
        sscanf(line,"%f %f %f\n",&a,&b,&c);
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
        read = getline(&line, &len, fp); //to discard every other line
    }
    fclose(fp);
    fann_destroy(ann);
    return 0;
}
