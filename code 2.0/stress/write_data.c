#include "write_data.h"
#include "result_structure.h"
#include <math.h>
#include <ncurses.h>
#include "control_server.h"

static long Max=0;
static long Min=999999999;
static long Avg=0;
static unsigned int Cycle=0;
int indexPlot=0;
unsigned int HistOverflow=0;
long usvalue=0;
 int sum=0 ;

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

void writeDataToText(long networkLatency) {
    fprintf(fp,"%ld\n",networkLatency);
}

void initarrayPlot(){
  for(int i=0; i<MAXPLOT;i++)
    arrayPlot[i]=0; 
}

void print_stat(long netLatency){
  
  if(Max < netLatency){
    Max=netLatency;
  }
  if(Min > netLatency){
    Min=netLatency; 
  }
  Avg+=netLatency;
  Cycle++;
  
  printf("\tC:%d Min:%ld Act:%ld Avg:%ld Max:%ld       \r"
	,Cycle,Min,netLatency,(long)Avg/Cycle,Max);
  fflush(stdout);

  indexPlot= (int)nearbyint(netLatency/1000);
  
  if(indexPlot<MAXPLOT){
    arrayPlot[indexPlot]++;
    
    }
    else HistOverflow++;
    
}

int toMicroSecond(long nanoNum){
  return (int)nearbyint(nanoNum/1000);
}

void writeDataForPlot(){

  printf("#Histogram\n");

  for (int index = 0; index < MAXPLOT; index++) {
    printf("%d\t%d\n",index, arrayPlot[index]);
  }
     
  printf("# Total: %d \n",Cycle);
  printf("# Min Latencies: %d \n",toMicroSecond(Min));
  printf("# Avg Latencies: %d \n",toMicroSecond(Avg/Cycle));
  printf("# Max Latencies: %d \n",toMicroSecond(Max));
  printf("# Histogrammme overflow... : %d\n", HistOverflow);
   
}

void writeDataForCDFPlot(){
  long i=0;
  printf("#Histogram\n");
  
  for (int index = 0; index < 3000; index++) {
    sum += arrayPlot[index];       
    i=(sum*100)/Cycle;
    printf("%d\t%d\n",index,(int)nearbyint(i));
    
  }
   
     
  printf("# Total: %d \n",Cycle);
  printf("# Min Latencies: %d \n",toMicroSecond(Min));
  printf("# Avg Latencies: %d \n",toMicroSecond(Avg/Cycle));
  printf("# Max Latencies: %d \n",toMicroSecond(Max));
  printf("# Histogrammme overflow... : %d\n", HistOverflow);
}
