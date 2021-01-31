/*

This header file is for controling the server socket

In this header, there are 7 global variables

s_src_addr          : Save the information about the sender who sent the message
s32_server_sock     : Save the server socket information
u16_server_data_off : Save the index of data part
pu8a_server_frame   : Save the entire message of frame including the header
pu8a_server_data    : Save the start point of data part
u32_src_addr_len    : Save the size of s_src_addr
sArr                : Save the information of message from client

sArr array is consist of TOKEN_NUM(:= 3) part
sArr[0] : Save the string
sArr[1] : Save the index
sArr[2] : Save the timestamp

pu8a_frame and pu8a_data variables are linked to each address of frame and payload

	pu8a_frame                      pu8a_data
	v                               v
	 ==============================================================
	| dest_mac | src_mac | protocol |       payload                |
	 ==============================================================

To use the server socket, you have to follow below steps

        init_server_frame()                     init_server_socket()                init_server_sockaddr_ll()
        : ERROR_CREATE_FRAME			: ERROR_CREATE_SOCKET				|
		|                                       |                                       |
                |                               set_server_socket()                  set_server_sockaddr_ll()
                |                                       |                                       |
                |                                        ---------------------------------------
                |                                                           |
                |                                                  bind_server_socket()
                |                                                   : ERROR_BIND_SOCKET
                |                                                           |
                -------------------------------------------------------------
                                                |
                                          receive_data()
                                          : FAIL_RECEIVE_DATA
                                          : ERROR_RECEIVE_DATA
                                                |
                                      check_data_from_target()
                                      : NOT_FROM_TARGET
                                      : FROM_TARGET
                                                |
                                           parse_data()
                                                |
                                        check_correct_data()
                                        : NOT_CORRECT_DATA
                                        : CORRECT_DATA
                                                |
                -----------------------------------------------------------------------------
                |                       |                           |                       |                   
    print_target_mac_addr()     print_packet_string()       print_packet_index()   print_packet_timestamp()


init_server_frame()         : Alloc the space for frame packet and Save the frame and data point
- The user can choose the size of packet using parameter
- Error code
ERROR_CREATE_FRAME          : If creating frame is failed, the function will return this error code

free_server_frame()         : Free the frame

init_server_socket()        : Initiate the socket to -1 (default value)

set_server_socket()         : Set the socket for raw packet
- Error code
ERROR_CREATE_SOCKET 		: If creating socket is failed, the function will return this error code

bind_server_socket()        : Bind the socket with sockaddr_ll structure
- Error code
ERROR_BIND_SOCKET           : If binding socket is failed, the function will return this error code

close_server_socket()       : Close the socket

init_server_sockaddr_ll()   : Initiate the sockaddr_ll to 0 (default value)

set_setver_sockaddr_ll()    : Set the sockaddr_ll for using specific network device and saving the destination MAC address
- To set this struct, you have to get NIC_INDEX and DEST_MAC_ADDR from outside

receive data()              : Receive the message from client
- Error code
FAIL_RECEIVE_DATA           : If receiving the message is failed, the function will return this error code
ERROR_RECEIVE_DATA          : If receiving the mssage is failed, the function will return this error code

check_data_from_target()    : Check the mesage is from the target or not checking the MAC address
- Flag code
NOT_FROM_TARGET             : If the message isn't from the target, the function will return this flag code
FROM_TARGET                 : If the message is from the target, the function will return this flag code

parse_data()                : Split the message to 3 part : String, Index, Timestamp

check_correct_data()        : Check the String is correct string or not
- Flag code
NOT_CORRECT_DATA            : If the message hasn't the correct string, the function will return this flag code
CORRECT_DATA : If the message has the correct string, the function will return this flag code

get_packet_string()         : Get the string parsed by parser
get_packet_index()          : Get the index parsed by parser
get_packet_timestamp()      : Get the timestamp parsed by parse

print_target_mac_addr()     : Print the target MAC address
print_packet_string()       : Print the string
print_packet_index()        : Print the index
print_packet_timestamp()    : Print the timestamp
*/

#include "main_header.h"
#include "common_header.h"

#define TOKEN_NUM               3

struct sockaddr_ll s_src_addr;
int32_t s32_server_sock;
uint16_t u16_server_data_off;
uint8_t *pu8a_server_frame;
uint8_t *pu8a_server_data;
socklen_t u32_src_addr_len;
char *sArr[TOKEN_NUM];

int init_server_frame(int frame_size);
void free_server_frame();

void init_server_socket();
int set_server_socket();
int bind_server_socket();
void close_server_socket();

void init_server_sockaddr_ll();
void set_server_sockaddr_ll(int32_t nic_index);

int receive_data(int frame_size);
int check_data_from_target(uint8_t* target_address);

void parse_data();
int check_correct_data(char* string);

char* get_packet_string();
char* get_packet_index();
char* get_packet_timestamp();

void print_target_mac_addr();
void print_packet_string();
void print_packet_index();
void print_packet_timestamp();
