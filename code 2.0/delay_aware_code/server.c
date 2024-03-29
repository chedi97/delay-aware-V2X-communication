#define _GNU_SOURCE
#include <sched.h>
#include <getopt.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "get_nic_index.h"
#include "write_data.h"
#include "get_configuration.h"

#define DEFAULT_RTPRIORITY 49
#define DEFAULT_PACKET_SIZE 60
#define TOKEN_NUM 4  

//client
struct sockaddr_ll s_dest_addr;
int32_t s32_client_sock;
uint16_t u16_client_data_off;
uint8_t *pu8a_client_frame;
uint8_t *pu8a_client_data;
//end_client

//server
struct sockaddr_ll s_src_addr;
int32_t s32_server_sock;
uint16_t u16_server_data_off;
uint8_t *pu8a_server_frame;
uint8_t *pu8a_server_data;
socklen_t u32_src_addr_len;
char *sArr[TOKEN_NUM];
int recv_data;
//end_server

int policy = SCHED_OTHER;	/* default policy if not specified */
int priority=0;
int main_affinity=0;
unsigned int finish=0;
int frame_size= DEFAULT_PACKET_SIZE; // 1514 bytes Max
int data_size = DEFAULT_PACKET_SIZE - ETH_HLEN; // ETH_HLEN 14 bytes
char* string= "raw_packet_test";
int diff;

void handlepolicy(char *polname)
{
  if (strncasecmp(polname, "other", 5) == 0)
    policy = SCHED_OTHER;
  else if (strncasecmp(polname, "fifo", 4) == 0){
    policy = SCHED_FIFO;
  }
  else if (strncasecmp(polname, "rr", 2) == 0)
    policy = SCHED_RR;
  else	/* default policy if we don't recognize the request */
    policy = SCHED_OTHER;
}

void process_options(int argc, char *argv[])
 {
   int error = 0;
   int max_cpus = sysconf(_SC_NPROCESSORS_CONF);
   cpu_set_t mask;
   pthread_t thread;
   for (;;) {
     int option_index = 0;
     /** Options for getopt */
     static struct option long_options[] = {
       {"packetsize", required_argument, NULL, 's'},
       {"affinity", required_argument, NULL, 'a'},
       {"policy", required_argument, NULL, 'y'},
       {"priority", required_argument, NULL, 'p'},
       {"help", no_argument, NULL, '?'},
       {NULL, 0, NULL, 0}
     };
     int c = getopt_long(argc, argv, "s:a:y:p:h:?",
			 long_options, &option_index);
     if (c == -1)
       break;
     
     switch (c) {
     case 'a':
       main_affinity= atoi(optarg);
       if (main_affinity != -1) {
	 CPU_ZERO(&mask);
	 CPU_SET(main_affinity, &mask);
	 thread = pthread_self();
	 if(pthread_setaffinity_np(thread, sizeof(mask), &mask) == -1)
	   printf("Could not set CPU affinity to CPU #%d\n", main_affinity);
       }
       if (main_affinity<0)
	 error = 1;	       
       break;
     case 's': frame_size= atoi(optarg);
       break;
     case 'y': 
       handlepolicy(optarg); break;
     case 'p':
       priority = atoi(optarg); 
       if (policy != SCHED_FIFO && policy != SCHED_RR)
	 policy = SCHED_FIFO;
       break;
     case '?': display_help(0); break;   
     }
   }
   
   if (main_affinity >= max_cpus) {
     printf("CPU #%d not found, only %d CPUs available\n",
	    main_affinity, max_cpus);
     error = 1;
   }
   if (priority < 0 || priority > 99)
       error = 1;
   if (priority && (policy != SCHED_FIFO && policy != SCHED_RR)) {
     fprintf(stderr, "policy and priority don't match: setting policy to SCHED_FIFO\n");
     policy = SCHED_FIFO;
   }
   if ((policy == SCHED_FIFO || policy == SCHED_RR) && priority == 0) {
     fprintf(stderr, "Defaulting realtime priority is set to %d\n",DEFAULT_RTPRIORITY);
     priority = DEFAULT_RTPRIORITY;
   }
   
   if (error)
     display_help(1);
   
 }

void free_client_frame() {
  free(pu8a_client_frame);
  pu8a_client_frame = NULL;
  pu8a_client_data  = NULL;
}

void close_client_socket() {
  if (s32_client_sock > 0) {
    close(s32_client_sock);
  }
}

void free_server_frame() {
  free(pu8a_server_frame);
  pu8a_server_frame = NULL;
  pu8a_server_data  = NULL;
}

void close_server_socket() {
  if (s32_server_sock > 0) {
    close(s32_server_sock);
  }
}

//////////////////////////////////////////////

int main(int argc, char *argv[])
{
  struct timespec server_send, server_recv;
  struct sched_param schedp;
  int8_t wrong_address;
  
  /*
    Initiate other modules : get_configuration.c , packet_size.c
  */
  if (get_configuration() != NO_ERROR) {
    return 0;
  }
  
  process_options(argc, argv);
  printf("Packet size is %d\n",frame_size);
  memset(&schedp, 0, sizeof(schedp));
  schedp.sched_priority = priority;
  sched_setscheduler(0, policy, &schedp);
  
  /*
    Initiate server socket
  */
  
  u16_server_data_off = (uint16_t) ETH_HLEN;
  pu8a_server_frame   = (uint8_t *) calloc(1, frame_size);
  
  if (pu8a_server_frame == NULL) {
    perror("Could not get memory for the transmit frame\n");
    return ERROR_CREATE_FRAME;
  }
  
  pu8a_server_data    = pu8a_server_frame + u16_server_data_off;

  //init_server_socket();
  s32_server_sock = -1;
  
  // init_server_sockaddr_ll();
  (void)memset(&s_src_addr, 0, sizeof(s_src_addr));
        
  //set_server_sockaddr_ll(get_nic_index(get_nic_name()));
  s_src_addr.sll_family = AF_PACKET;
  /*we don't use a protocol above ethernet layer, just use anything here*/
  s_src_addr.sll_protocol = htons(ETH_P_ALL);
  s_src_addr.sll_ifindex = get_nic_index(get_nic_name());
  s_src_addr.sll_hatype = ARPHRD_ETHER;
  s_src_addr.sll_pkttype = PACKET_OTHERHOST; //PACKET_OUTGOING
  s_src_addr.sll_halen = ETH_ALEN;
  
  socklen_t u32_src_addr_len = sizeof(s_src_addr);
  
  s32_server_sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));  
  if (s32_server_sock == -1) {
    perror("Could not create the socket");
    free_server_frame();
    return 0;
  }
  
  printf("Receive Socket created\n");
  
  int res = bind(s32_server_sock,
                 (struct sockaddr *)&s_src_addr,
                 sizeof(s_src_addr));
  
  if (res == -1) {
    perror("Could not bind to the socket");
    free_server_frame();
    close_server_socket();
    return 0;
  }
  
  
  /*
    Initiate client socket
  */
  /* init client frame */   
  u16_client_data_off = (uint16_t) ETH_HLEN;
  pu8a_client_frame   = (uint8_t *) calloc(1, frame_size);
  
  if (pu8a_client_frame == NULL) {
    perror("Could not get memory for the transmit frame\n");
    return 0;
  }
  
  pu8a_client_data    = pu8a_client_frame + u16_client_data_off;
  
  /* set client frame header */
  (void)memcpy(pu8a_client_frame, get_src_addr() , ETH_ALEN);
  (void)memcpy(pu8a_client_frame + ETH_ALEN, get_dest_addr() , ETH_ALEN);
  
  
  /* init_client_socket */
  s32_client_sock = -1;
        
  /* init client sockaddr_ll*/
  (void)memset(&s_dest_addr, 0, sizeof(s_dest_addr));
  
  /* set client socket */
  s32_client_sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
  if (s32_client_sock == -1) {
    perror("Could not create the socket");
          
    return 0;
  }
  printf("Send Socket created\n\n");
  
  /* set client sockaddr_ll */
  
  s_dest_addr.sll_family = AF_PACKET;
  /*we don't use a protocol above ethernet layer, just use anything here*/
  s_dest_addr.sll_protocol = htons(ETH_P_ALL);
  s_dest_addr.sll_ifindex = get_nic_index(get_nic_name());
  s_dest_addr.sll_hatype = ARPHRD_ETHER;
  s_dest_addr.sll_pkttype = PACKET_OTHERHOST; //PACKET_OUTGOING
  s_dest_addr.sll_halen = ETH_ALEN;
  /*MAC - begin*/
  s_dest_addr.sll_addr[0] = get_src_addr()[0];
  s_dest_addr.sll_addr[1] = get_src_addr()[1];
  s_dest_addr.sll_addr[2] = get_src_addr()[2];
  s_dest_addr.sll_addr[3] = get_src_addr()[3];
  s_dest_addr.sll_addr[4] = get_src_addr()[4];
  s_dest_addr.sll_addr[5] = get_src_addr()[5];
  /*MAC - end*/
  s_dest_addr.sll_addr[6] = 0x00; /*not used*/
  s_dest_addr.sll_addr[7] = 0x00; /*not used*/
  
  while (1)
    {
      /*
        Receive the message from the target
      */
      
      (void)memset(&s_src_addr, 0, sizeof(s_src_addr));
      recv_data = recvfrom(s32_server_sock,
                           pu8a_server_frame,
                           frame_size,
                           0,
                           (struct sockaddr *)&s_src_addr,
                           &u32_src_addr_len);
      
      if (recv_data == -1) {
        perror("Socket receive failed");
        free_server_frame();
        close_server_socket();
        return 0 ; //FAIL_RECEIVE_DATA;
      }
      
      else if (recv_data < 0) {
        perror("Socket receive, error ");
        free_server_frame();
        close_server_socket();
        return 0 ; //ERROR_RECEIVE_DATA;
      }
      
      wrong_address=0;     
      clock_gettime(CLOCK_MONOTONIC_RAW, &server_recv);
            
      for (int index = 0; index < sizeof(s_src_addr.sll_addr) - 2; index++) {
        if (s_src_addr.sll_addr[index] != get_src_addr()[index]) { 
          wrong_address=1 ;
          break; 
        }
      }
      
      if (!wrong_address) {
        char* messageToken = NULL;
        
        for (int index = 0; index < TOKEN_NUM; index++) {
          sArr[index] = 0;
        }
        
        messageToken = strtok(&pu8a_server_frame[u16_server_data_off], " ");
        
        for (int index = 0; index < TOKEN_NUM; index++) {
          sArr[index] = messageToken;
          messageToken = strtok(NULL, " ");
        }
       
        if ((strcmp(sArr[0],string)==0 ) &&  (atol(sArr[2]) >=  0)) {
            
          printf("   Packet index: %d | Sent time: %d ns       \r"
                 ,atoi(sArr[1]),atoi(sArr[2]),sArr[0]);
          fflush(stdout);
          
          /*
            Send the message back to the target
          */
          (void)memset(&pu8a_client_frame[u16_client_data_off], '\0',data_size);   
          clock_gettime(CLOCK_MONOTONIC_RAW, &server_send);
            
          diff= server_send.tv_nsec - server_recv.tv_nsec ;
          if (diff < 0) {
            diff = diff + 1000000000;
          }
          (void)snprintf((char *)&pu8a_client_frame[u16_client_data_off],
                         data_size,"%s %d %ld %d","Index/Diff" , atoi(sArr[1]), diff, recv_data);
          
          
          int send_result = sendto(s32_client_sock,
                                   pu8a_client_frame,
                                   frame_size,
                                   0,
                                   (struct sockaddr *)&s_dest_addr,
                                   sizeof(s_dest_addr));
          
          
          if (send_result == -1) {
            perror("Socket send failed");
            free_client_frame();
            close_client_socket();
            return 0;
          }
          
        }
      }
      
    }
  
  return 0;
}
