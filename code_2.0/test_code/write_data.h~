/*
Coded by Hyunjae Lee and Jihun Lim

This header file is for saving the latencys to text file
*/
#include "common_header.h"
#include "main_header.h"
#define MAXPLOT    10000 


FILE * fp;

int openTextFile(char* fileName);
void closeTextFile();

void writeDataForPlot(int max_latency, int min_latency, int avg,int cycle, int hist_overflow, uint64_t *array_plot);
void writeDataForCDFPlot(int max_latency, int min_latency, int avg,int cycle, int hist_overflow, uint64_t *array_plot);
void display_help(int error);
