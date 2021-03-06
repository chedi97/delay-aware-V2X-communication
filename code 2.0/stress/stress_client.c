
#define _GNU_SOURCE
#include <signal.h>
#include <sched.h>
#include <getopt.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "result_structure.h"
#include "write_data.h"
#include "get_nic_index.h"
#include "packet_size.h"
#include "control_client.h"
#include "control_server.h"
#include "get_configuration.h"       
#define DEFAULT_RTPRIORITY 49 

void *sock_recv_thread();
unsigned int finish=0;
int policy = SCHED_OTHER;	/* default policy if not specified */
int priority=0;
int histogram=0;
int main_affinity=0;
int thread_affinity=0;  

void handlepolicy(char *polname)
{
  if (strncasecmp(polname, "other", 5) == 0)
    policy = SCHED_OTHER;
  else if (strncasecmp(polname, "fifo", 4) == 0){
    policy = SCHED_FIFO;
  }
  else if (strncasecmp(polname, "rr", 2) == 0)
    policy = SCHED_RR;
  else	                       /* default policy if we don't recognize the request */
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
      {"main_affinity", required_argument, NULL, 'a'},
      {"thread_affinity", required_argument, NULL, 'b'},
      {"policy", required_argument, NULL, 'y'},
      {"priority", required_argument, NULL, 'p'},
      {"histogram", optional_argument, NULL, 'h'},
      {"help", no_argument, NULL, '?'},
      {NULL, 0, NULL, 0}
		};
    int c = getopt_long(argc, argv, "s:a:b:y:p:h::?",
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
      if (thread_affinity < 0)
	error = 1;	
      break;
    case 'b': thread_affinity= atoi(optarg);break;
    case 's': set_packet_size(optarg);break;
    case 'y': handlepolicy(optarg); break;
    case 'p':
      priority = atoi(optarg); 
      if (policy != SCHED_FIFO && policy != SCHED_RR)
	policy = SCHED_FIFO;
      break;
    case 'h':
      if(optarg!=NULL)
	histogram=atoi(optarg);
      else 
	histogram=2;
      break; 
    case '?': display_help(0); break;   
    }
  }
  
		
  if (main_affinity >= max_cpus) {
    printf("CPU #%d not found, only %d CPUs available\n",
	   main_affinity, max_cpus);
    error = 1;
  }
  if (thread_affinity >= max_cpus) {
    printf("CPU #%d not found, only %d CPUs available\n",
	   thread_affinity, max_cpus);
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

static void sighand(int sig)
{
  finish=1;
  if(histogram==1)
    writeDataForCDFPlot();
  else if(histogram==2)
    writeDataForPlot();
  else printf("\n\n");
  
}

int main(int argc, char *argv[])
{
  struct timespec client_send,next;
  struct sched_param schedp;
  uint16_t u16_i = 0;
  pthread_t ul_recv_thd_id = -1;
    

  /*
    Initiate other modules : result_structure.c , get_configuration.c , write_data.c , packet_size.c
  */
  signal(SIGINT, sighand);
  
  initArray();
  initarrayPlot();
  
  if (get_configuration() != NO_ERROR) {
    return 0;
  }
  
  set_packet_size("60");
  process_options(argc, argv);
  printf("Packet size is %d\n",get_packet_size(1));
  memset(&schedp, 0, sizeof(schedp));
  schedp.sched_priority = priority;
  sched_setscheduler(0, policy, &schedp);
     
  
  /*
    Initiate client socket
  */
  if(init_client_frame(get_packet_size(FRAME)) != NO_ERROR) {
    return 0;
  }
  
  set_client_frame_header(get_src_addr(), get_dest_addr());
  init_client_socket();
  init_client_sockaddr_ll();
  
  if (set_client_socket() != NO_ERROR) {
    free_client_frame();
    return 0;
  }
  set_client_sockaddr_ll(get_nic_index(get_nic_name()), get_dest_addr());
  
  /*
    Create pthread for receiving the message
  */
  
  /* (void)pthread_create(&ul_recv_thd_id, NULL, sock_recv_thread, NULL);
     sleep(1);*/
  
  printf("Sending data using raw socket over  %s \n\n", get_nic_name());
  
  clock_gettime(CLOCK_MONOTONIC, &next);
  
  while (1)
    {
      /*
	Sending the message to the server
      */
      init_client_data(get_packet_size(DATA));
      
      clock_gettime(CLOCK_MONOTONIC_RAW, &client_send);
      putSendTime(client_send.tv_nsec, u16_i);
      
      set_client_data(get_packet_size(DATA), "raw_packet_test",	\
		      u16_i++,					\
		      client_send.tv_nsec);

      if (send_client_data(get_packet_size(FRAME)) != NO_ERROR) {
	free_client_frame();
	close_client_socket();
	
	return 0;
      }
          
      
      // Normalize the time to account for the second boundary
      if(next.tv_nsec >= 1000000000) {
      	next.tv_nsec -= 1000000000;
      	next.tv_sec++;
      }else next.tv_nsec += 50000; // 45000 -> 100%
      clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL);
      
      //usleep(1);
      
      if(finish){
	return 0;
      }
      
    }
  
  return 0;
}


