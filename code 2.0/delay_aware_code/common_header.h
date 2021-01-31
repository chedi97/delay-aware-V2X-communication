/*

This header file is for including common library
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#define NO_ERROR                0

/*
This error code will be used in packet_size.c
The function will return this error code according to below condifion

ERROR_WORD : if the user input the wrong word as parameter
*/

#define ERROR_WORD              1

/*
This error code will be used in get_configuration.c and write_data.c
The function will return this error code according to below condifion

ERROR_OPEN_FILE  : if the user didn't make configuration.txt or if the program can't open the file
ERROR_WRONG_MAC  : if the user input the wrong mac address as parameter
ERROR_WRONG_FRE  : if the user input the wrong frequency as parameter
ERROR_WRONG_BAND : if the user input the wrong bandwidth as parameter
*/

#define ERROR_OPEN_FILE         2

#define ERROR_WRONG_MAC         3
#define ERROR_WRONG_FRE         4
#define ERROR_WRONG_BAND        5