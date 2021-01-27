#include <math.h>
#include <ncurses.h>
#include "write_data.h"

char command[255];

int openTextFile(char* fileName) {
  fp = fopen(fileName,"w");
  
  if (fp == NULL) {
    perror("Cannot open the file\n");
    
    return ERROR_OPEN_FILE;
  }
  
  return NO_ERROR;
}

void closeTextFile() {
  fclose(fp);
}

int toMicroSecond(long nanoNum){
  return (int)nearbyint(nanoNum/1000);
}

void writeDataForPlot(int max_latency, int min_latency, int avg,int cycle, int hist_overflow, uint64_t *array_plot){
  /*openTextFile("plot/output");
  fprintf(fp,"#Histogram\n");
  
  for (int index = 0; index < MAXPLOT; index++) {
    
    fprintf(fp,"%d\t%d\n",index, array_plot[index]);
  }
  
  fprintf(fp,"# Total: %d \n",cycle);
  fprintf(fp,"# Min Latencies: %d \n",toMicroSecond(min_latency));
  fprintf(fp,"# Avg Latencies: %d \n",toMicroSecond(avg/cycle));
  fprintf(fp,"# Max Latencies: %d \n",toMicroSecond(max_latency));
  fprintf(fp,"# Histogrammme overflow... : %d\n", hist_overflow);
  closeTextFile();
  sprintf(command, "sudo ./mklatencyplot");
  system(command);*/

  //for tikz
   openTextFile("plot/output.dat");

   fprintf(fp,"L\tP\n");
   
   for (int index = 0; index < MAXPLOT; index++) {
     fprintf(fp,"%d\t%d\n",index, array_plot[index]);
   }
   
}

void writeDataForCDFPlot(int max_latency, int min_latency, int avg, int cycle,int hist_overflow, uint64_t *array_plot) {
   long i=0;
  int sum=0;
/*openTextFile("plot/output");
  fprintf(fp,"#Histogram\n");
  
  for (int index = 0; index < MAXPLOT; index++) {
    sum += array_plot[index];       
    i=(sum*100)/cycle;
    fprintf(fp,"%d\t%d\n",index,(int)nearbyint(i));
    
  }
  
  
  fprintf(fp,"# Total: %d \n",cycle);
  fprintf(fp,"# Min Latencies: %d \n",toMicroSecond(min_latency));
  fprintf(fp,"# Avg Latencies: %d \n",toMicroSecond(avg/cycle));
  fprintf(fp,"# Max Latencies: %d \n",toMicroSecond(max_latency));
  fprintf(fp,"# Histogrammme overflow... : %d\n", hist_overflow);
  closeTextFile();
  sprintf(command, "sudo ./mklatencyplot");
  system(command);*/

  // for tikz
  openTextFile("plot/output.dat");

  fprintf(fp,"L\tP\n");
  for (int index = 0; index < MAXPLOT; index++) {
    sum += array_plot[index];       
    i=(sum*100)/cycle;
    fprintf(fp,"%d\t%d\n",index,(int)nearbyint(i));
    
  }  
  
}

void display_help(int error)
{
  	printf("Usage:\n"
	       "client <options>\n\n"
	       "-s [NUM] --packet_size            set the packet size to be sent and received\n"
	       "-a       --main_affinity= N       run main thread on processor #N\n"
	       "			 	  sending thread will run on the same processor\n"
	       "				  if -b is not specified\n"
	       "-b       --thread_affinity= N     run sending thread on precessor #N\n"
	       "-y POLI  --policy= POLI           policy of realtime thread, POLI may be fifo(default) or rr\n"
	       "                                  format: --policy=fifo(default) or --policy=rr\n"
	       "-p PRIO  --priority= PRIO         set priority of program's thread\n"
               "-c       --cycle = CYC            set number of maximum cycle 'CYC'             \n "
               "-t       --period= PER            set period duration in ms \n"
               "                                   default period is 10 ms  \n"
               "-r       --run_time= RUN          set run_time duration in ms \n"
	       "-h	 --histogram         	  display histogram\n"
	       "-h HIS   --histogram=HIS     	  set HIS to 1 to display CDF histogram\n\n");
	
	if(error)
	  exit(EXIT_FAILURE);
        exit(EXIT_SUCCESS);
	
}
