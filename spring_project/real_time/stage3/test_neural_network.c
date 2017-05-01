#include <unistd.h>

#include <stdio.h>
#include "floatfann.h"

#include <string.h>
#include <stdlib.h>
#include <sys/file.h>
#include <signal.h>
#include <unistd.h>

sig_atomic_t volatile run_flag = 1;

void do_when_interrupted(int sig)
{
	if(sig == SIGINT)
		run_flag = 0;
}

int main()
{
    int i, nfeatures;
    int speed;
   
    float max;
    fann_type *calc_out;
    fann_type input[9];
    struct fann *ann;
    ann = fann_create_from_file("./stage3/TEST.net");

    FILE *fp;
    char file_name[100];
    float features[9];
    char *line = NULL;
    size_t len;
    ssize_t read;
    
    int nOutputs = 3;
    int cntr = 0;
    char number[10];
    char file[10] = "file";
    char extension[10] = ".txt";
    int fd;
    while(1)
    {
	len = 0;
	signal(SIGINT,&do_when_interrupted);
	if(!run_flag){ break;}
	sprintf(number,"%d",cntr);
	strcpy(file_name,file);
	strcat(file_name,number);
	strcat(file_name,extension);
        fp = fopen(file_name,"r");
	if (fp == NULL)
	{
		continue;
		fprintf(stderr,"Failed to open file \'%s\'. Exiting.\n",
				file_name);
		exit(EXIT_FAILURE);

	}
	fd = fileno(fp);
	flock(fd,LOCK_EX);
        
	//read = getline(&line, &len, fp); //discard the frist line
	while ((read = getline(&line, &len, fp)) != -1) {
		sscanf(line,"%f %f %f %f %f %f %f %f %f\n",(features),
			(features+1),(features+2),(features+3),(features+4),
			(features+5),(features+6),(features+7),(features+8));
	
		for(nfeatures=0;nfeatures<9;nfeatures++)
		{
      			input[nfeatures] = (float) features[nfeatures];

		}
	        calc_out = fann_run(ann, input);

       		max = -100;
	        for (i = 0; i < nOutputs; i++) {
        	    if (calc_out[i] > max) {
	                max = calc_out[i];
        	        speed = i+1;
	            }
       		 }

		printf("speed is %d\n", speed);
	        sleep(1);
	        read = getline(&line, &len, fp); //to discard every other line
 	}
	    fclose(fp);
	    remove(file_name);
	    cntr++;

    }
    fann_destroy(ann);
    return 0;
}
