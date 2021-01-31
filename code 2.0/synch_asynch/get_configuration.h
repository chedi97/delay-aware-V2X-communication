/*
This header file is for getting the values of configuration.txt

In configuration.txt,

There are 5 values in each row
NIC_NAME:
SRC_MAC:
DEST_MAC:
FREQUENCY:
BANDWIDTH:

The user can save the values using get_configuration() function
And use the values using the function started with get_* function
*/
#include "common_header.h"
#include "main_header.h"

#define MAC_ADDR_LEN        6

char* NIC_NAME;
uint8_t gu8a_src_mac[MAC_ADDR_LEN];
uint8_t gu8a_dest_mac[MAC_ADDR_LEN];
int frequency;
int bandwidth;

int get_configuration();

char* get_nic_name();
uint8_t* get_src_addr();
uint8_t* get_dest_addr();
int get_frequency();
int get_bandwitch();
