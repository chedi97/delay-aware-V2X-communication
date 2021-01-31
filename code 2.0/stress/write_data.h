/*

This header file is for saving the latencys to text file
*/

#include "common_header.h"
#define MAXPLOT    8000

uint64_t arrayPlot[MAXPLOT];


FILE * fp;
 

int openTextFile(char* fileName);
void closeTextFile();

void writeDataToText(long networkLatency);
void writeDataForPlot();
void writeDataForCDFPlot();
void print_stat(long netLatency);
void initarrayPlot();
