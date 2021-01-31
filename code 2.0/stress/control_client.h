/*

This header file is for controling the client socket

In this header, there are 5 global variables

s_dest_addr         : Save the network information including the destination mac address
s32_client_sock     : Save the client socket information
u16_client_data_off : Save the index of data part
pu8a_client_frame   : Save the entire message of frame including the header
pu8a_client_data    : Save the start point of data part

pu8a_frame and pu8a_data variables are linked to each address of frame and payload

	pu8a_frame                      pu8a_data
	v                               v
	 ==============================================================
	| dest_mac | src_mac | protocol |       payload                |
	 ==============================================================

To use the client socket, you have to follow below steps

        init_client_frame()                     init_client_socket()                init_client_sockaddr_ll()
        : ERROR_CREATE_FRAME			: ERROR_CREATE_SOCKET				|
		|                                       |                                       |
     set_client_frame_header()                  set_client_socket()                  set_client_sockaddr_ll()
                |                                       |                                       |
                ---------------------------------------------------------------------------------
							|
						init_client_data()
							|
						set_client_data()
							|
						send_client_data()
						: ERROR_SEND_MESSAGE


init_client_frame() 		: Alloc the space for frame packet and Save the frame and data point
- The user can choose the size of packet using parameter
- Error code
ERROR_CREATE_FRAME : If creating frame is failed, the function will return this error code

set_client_frame_header() 	: Input the source and destination MAC address to frame header

free_client_frame() 		: Free the frame

init_client_socket() 		: Initiate the socket to -1 (default value)

set_client_socket() 		: Set the socket for raw packet
- Error code
ERROR_CREATE_SOCKET 		: If creating socket is failed, the function will return this error code

close_client_socket()		: Close the socket

init_client_sockaddr_ll() 	: Initiate the sockaddr_ll to 0 (default value)

set_client_sockaddr_ll() 	: Set the sockaddr_ll for using specific network device and saving the destination MAC address
- To set this struct, you have to get NIC_INDEX and DEST_MAC_ADDR from outside

init_client_data() 			: Initiate the data part to 0 (default value)
- The user can choose the size of data using parameter

set_client_data() 			: Input the message to data part
- In the data part, current format is consist of String + Index + Timestamp 

send_client_data() 			: Send the message to target
- Error code
ERROR_SEND_MESSAGE 			: If sending the message is failed, the function will return this error code
                                                
*/
#include "main_header.h"
#include "common_header.h"

struct sockaddr_ll s_dest_addr;
int32_t s32_client_sock;
uint16_t u16_client_data_off;
uint8_t *pu8a_client_frame;
uint8_t *pu8a_client_data;

int init_client_frame(int frame_size);
void set_client_frame_header(uint8_t* src_addr, uint8_t* dest_addr);
void free_client_frame();

void init_client_data(int data_size);
void set_client_data(int data_size, char* string, int index, long timestamp);

void init_client_socket();
int set_client_socket();
void close_client_socket();

void init_client_sockaddr_ll();
void set_client_sockaddr_ll(int32_t nic_index, uint8_t* dest_addr);

int send_client_data(int frame_size);
void display_help(int error);

