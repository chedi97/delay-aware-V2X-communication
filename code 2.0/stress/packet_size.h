/*
Coded by Hyunjae Lee and Jihun Lim

This header file is for setting and getting the packet size

The user can choose the length of packet using LARGE and SMALL argument

If the user use LARGE argument, 
frame_size will be ETH_FRAME_LEN        : 1514
data_size will be ETH_DATA_LEN          : 1500

If the user use SMALL argument,
frame_size will be ETH_ZLEN             : 60
data_size will be ETH_ZELN - ETH_HLEN   : 46

The user can choose the length of packet using set_packet_size() function
And get the length of packet using get_packet_size() function

If the user input FRAME value as option parameter, 
The function will return frame_size

If the user input DATA value as option parameter,
The function will return data_size
*/

#include "common_header.h"
#include <net/ethernet.h> /* the L2 protocols */

#define FRAME       1
#define DATA        2

int data_size, frame_size;

int set_packet_size(char *packet_size);
int get_packet_size(int option);
