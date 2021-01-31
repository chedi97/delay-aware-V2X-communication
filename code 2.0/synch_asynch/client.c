/*
 Coded by Hyunjae Lee and Jihun Lim
 
 This is a simple implementation for latency evaluation in a V2V network.
 We assume that all node knows other MAC addresses who want to communicate with. 

 [Client]
 : Client works as a sender, it sends a message regurally to a specific node with MAC address which The client already know.

  *Reference : https://stackoverflow.com/questions/10824827/raw-sockets-communication-over-wifi-receiver-not-able-to-receive-packets
*/
#define _GNU_SOURCE

#include <signal.h>
#include <sched.h>
#include <getopt.h>
#include <pthread.h>
#include <errno.h>
#include <math.h>
#include <inttypes.h>

#include "get_nic_index.h"
#include "write_data.h"
#include "get_configuration.h"
#include "sched_edf.h"

#define DEFAULT_RTPRIORITY 49 
#define DEFAULT_PACKET_SIZE_SYNCH 614
#define DEFAULT_PACKET_SIZE_ASYNCH 814
#define DEFAULT_PERIOD 10 
#define DEFAULT_RUNTIME 5 
#define MAXIMUM 100
#define TOKEN_NUM 4              
#define NO_ERROR 0

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

unsigned int finish=0;
int policy = SCHED_OTHER;	/* default policy if not specified */
int priority=0;
int max_cycle=0;
int histogram=0;
int period = DEFAULT_PERIOD; 
int main_affinity=0;
int thread_affinity=0;
int synch_frame_size= DEFAULT_PACKET_SIZE_SYNCH; // 1514 bytes Max
int asynch_frame_size= DEFAULT_PACKET_SIZE_ASYNCH ; 
int synch_data_size = DEFAULT_PACKET_SIZE_SYNCH - ETH_HLEN; // ETH_HLEN 14 bytes  // CAM : 100 to 600 bytes
int asynch_data_size = DEFAULT_PACKET_SIZE_ASYNCH - ETH_HLEN; // DENM : 300 to 800 bytes 
uint64_t network_latency;
int diff;
char* string= "Index/Diff";
int index_array;
int run_time = 0;
int  recv_frame_size = 60 ;

// plot
uint64_t max_latency=0;
long min_latency=999999999;
long avg=0;
unsigned int cycle=0;
int index_plot=0;
int hist_overflow=0;
uint64_t array_plot[MAXPLOT]= {0} ;
//end plot

struct timeGap
{
  long sendTime;
  long recvTime;
  long diff;
};

struct timeGap circular_array[MAXIMUM];


void *sock_send_synch_thread();
void *sock_send_asynch_thread();

void handlepolicy(char *polname)
{
  if (strncasecmp(polname, "other", 5) == 0)
    policy = SCHED_OTHER;
  else if (strncasecmp(polname, "fifo", 4) == 0)
    policy = SCHED_FIFO;
  else if (strncasecmp(polname, "rr", 2) == 0)
    policy = SCHED_RR;
  else if (strncasecmp(polname, "edf", 3) == 0)
    policy = SCHED_DEADLINE;
    
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
      {"packet_size", required_argument, NULL, 's'},
      {"main_affinity", required_argument, NULL, 'a'},
      {"thread_affinity", required_argument, NULL, 'b'},
      {"policy", required_argument, NULL, 'y'},
      {"priority", required_argument, NULL, 'p'},
      {"cycle", required_argument, NULL, 'c'},
      {"period", required_argument, NULL, 't'},
      {"run_time", required_argument,NULL, 'r'},
      {"histogram", optional_argument, NULL, 'h'},      
      {"help", no_argument, NULL, '?'},
      {NULL, 0, NULL, 0}
		};
    int opt = getopt_long(argc, argv, "s:a:b:y:p:c:t:r:h::?",
			long_options, &option_index);
    if (opt == -1)
      break;
    
    switch (opt) {
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
    case 'b': thread_affinity= atoi(optarg);
      if (thread_affinity < 0)
	error = 1;
      break;
      /* case 's':
      frame_size= atoi(optarg);
      data_size= atoi(optarg) - ETH_HLEN;
      break;*/
    case 'y': handlepolicy(optarg); break;
    case 'p':
      priority = atoi(optarg); 
      if (policy != SCHED_FIFO && policy != SCHED_RR)
	policy = SCHED_FIFO;
      break;
    case 'c':
      max_cycle=atoi(optarg);
      if(atoi(optarg)<=0)
        error = 1;
      break;
    case 'h':
      if(optarg!=NULL)
	histogram=atoi(optarg);
      else 
	histogram=2;
      break;
    case 't':
      period = atoi(optarg);
      break;
    case 'r':
      run_time = atoi(optarg);
      break;
    case '?': display_help(0); break;   
    }
  }
  /* if(frame_size<ETH_ZLEN || frame_size>ETH_FRAME_LEN){
    perror("You need to put packet size between 60 and 1514 \n");
    exit(EXIT_FAILURE);
    }*/
  
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
  if (policy == SCHED_DEADLINE && run_time == 0 ) {
    fprintf(stderr, "Defaulting run time is set to %d ms / period %d ms \n", DEFAULT_RUNTIME,period);
    run_time = DEFAULT_RUNTIME;
  }
  if(policy!= SCHED_DEADLINE && run_time!=0 ) {
    fprintf(stderr, "policy and run time don't match: setting policy to SCHED_DEADLINE\n");
    policy = SCHED_DEADLINE;
  }
  
  if(period <= 0)
    error=1;
  if(run_time < 0)
    error=1;
  
  if (error)
    display_help(1); 
}


struct timespec fn_diff(struct timespec start, struct timespec end)
{
  struct timespec temp;
  if ((end.tv_nsec - start.tv_nsec)<0) {
    temp.tv_sec = end.tv_sec - start.tv_sec-1;
    temp.tv_nsec = 1000000000 + end.tv_nsec-start.tv_nsec;
  } else {
    temp.tv_sec = end.tv_sec - start.tv_sec;
    temp.tv_nsec = end.tv_nsec - start.tv_nsec;
  }
  return temp;
}

void free_client_frame(uint8_t *pu8a_client_frame ,uint8_t *pu8a_client_data ) {
    free(pu8a_client_frame);
    pu8a_client_frame = NULL;
    pu8a_client_data  = NULL;
}

void close_client_socket(int32_t *s32_client_sock) {
    if (*s32_client_sock > 0) {
        close(*s32_client_sock);
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

int my_rand (void)
{
   static int first = 0;
   
   if (first == 0)
   {
      srand (time (NULL));
      first = 1;
   }
   return (rand()%15000000)+5000000;
}

/* for EDF */
int sched_setattr(pid_t pid, const struct sched_attr *attr, unsigned int flags)
{
  return syscall(__NR_sched_setattr, pid, attr, flags);
}

int sched_getattr(pid_t pid, struct sched_attr *attr, unsigned int size, unsigned int flags)
{
  return syscall(__NR_sched_getattr, pid, attr, size, flags);
}
/*end EDF fnct */

static void sighand(int sig)
{
  finish=1;
  /* free_client_frame();
  close_client_socket();
  free_server_frame();
  close_server_socket();*/
  
  if(histogram==1){
    printf("\n\n");
    writeDataForCDFPlot(max_latency,min_latency,avg,cycle,hist_overflow,array_plot);
  }
  else if(histogram==2){
    printf("\n\n");
    writeDataForPlot(max_latency,min_latency,avg,cycle,hist_overflow,array_plot);
  }
  else printf("\n\n");
  
}


///////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[])
{
  
  struct sched_attr attr;
  pthread_t send_synch_thd_id = -1;
  pthread_t send_asynch_thd_id = -1;
  struct sched_param schedp;
  
  printf("Code 2.0 \n");
  signal(SIGINT, sighand);
  
  /* initArray */
  (void)memset(&circular_array, 0, sizeof(circular_array));
  
  if (get_configuration() != NO_ERROR) {
    return 0;
  }
  
  process_options(argc, argv);
  //  printf("Packet size is %d\n", frame_size);

  
  /* set scheduler */
  memset(&schedp, 0, sizeof(schedp));
  schedp.sched_priority = priority;
  sched_setscheduler(0, policy, &schedp);
  period = period * 1000 * 1000 ; 
 


(void)pthread_create(&send_synch_thd_id, NULL, sock_send_synch_thread, NULL);
sleep(1);
(void)pthread_create(&send_asynch_thd_id, NULL, sock_send_asynch_thread, NULL);
sleep(1);

////////////////////////////////////////////////////////// 
 
    struct timespec client_recv; 
    cpu_set_t mask;
    pthread_t thread;
    int8_t wrong_address;

    
    /*
      Initiate server socket
    */
    
    /* If thread_affinity is not specified, it will take the same affinity of the main */
    
    if (thread_affinity != -1) {
      CPU_ZERO(&mask);
      CPU_SET(thread_affinity, &mask);
      thread = pthread_self();
      if(pthread_setaffinity_np(thread, sizeof(mask), &mask) == -1)
        printf("Could not set CPU affinity to CPU #%d\n", thread_affinity);
    }
    
    /* init_server_frame */
   
    u16_server_data_off = (uint16_t) ETH_HLEN;
    pu8a_server_frame   = (uint8_t *) calloc(1, recv_frame_size);
    
    if (pu8a_server_frame == NULL) {
      perror("Could not get memory for the transmit frame\n");
      return 0; // ERROR_CREATE_FRAME;
    }
    
    pu8a_server_data    = pu8a_server_frame + u16_server_data_off;
    
    /* init_server_socket */
    s32_server_sock = -1;
    
    /* init_server_sockaddr_ll */ 
    (void)memset(&s_src_addr, 0, sizeof(s_src_addr));
    
    /* set_server_sockaddr_ll */
     s_src_addr.sll_family = AF_PACKET;
     /*we don't use a protocol above ethernet layer, just use anything here*/
     s_src_addr.sll_protocol = htons(ETH_P_ALL);
     s_src_addr.sll_ifindex = get_nic_index(get_nic_name());
     s_src_addr.sll_hatype = ARPHRD_ETHER;
     s_src_addr.sll_pkttype = PACKET_OTHERHOST; //PACKET_OUTGOING
     s_src_addr.sll_halen = ETH_ALEN;
     
     socklen_t u32_src_addr_len = sizeof(s_src_addr);

     /* Create socket */ 
     s32_server_sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
     
     if (s32_server_sock == -1) {
       perror("Could not create the socket");
       free_server_frame();
       return 0; //ERROR_CREATE_SOCKET;
     }
     
     printf("Receive Socket created\n");

     /* Assign the addr to the socket */ 
     if ( bind(s32_server_sock, (struct sockaddr *)&s_src_addr, sizeof(s_src_addr))== -1) {
       free_server_frame();
       close_server_socket();
       perror("Could not bind to the socket");     
       return 0 ; //ERROR_BIND_SOCKET;
     }
     
     /////////////////////////////////////////////////////////////////////////////////
     
     /*
       Receive the message from target
     */
          
     while (1)
       {          
         /* init server sockaddr */
         (void)memset(&s_src_addr, 0, sizeof(s_src_addr));
         recv_data = recvfrom(s32_server_sock,
                              pu8a_server_frame,
                              recv_frame_size,
                              0,
                              (struct sockaddr *)&s_src_addr,
                              &u32_src_addr_len);
         
         if (recv_data == -1) {
           perror("Socket receive failed");
           free_server_frame();
           close_server_socket();
           return 0 ;// FAIL_RECEIVE_DATA;
         }
         
         else if (recv_data < 0) {
           free_server_frame();
           close_server_socket();
           perror("Socket receive, error");
           exit(EXIT_FAILURE) ; //ERROR_RECEIVE_DATA;
         }
         
         wrong_address=0;
         clock_gettime(CLOCK_MONOTONIC_RAW, &client_recv);
         
         /* check data from target addr */
         
         for (int index = 0; index < sizeof(s_src_addr.sll_addr) - 2; index++) {
           if (s_src_addr.sll_addr[index] != get_dest_addr()[index]) {
             wrong_address=1 ;
             break; 
           }
          }
         
         /* prase data */
         
         if(!wrong_address) {
          char* messageToken = NULL;
          
          for (int index = 0; index < TOKEN_NUM; index++) {
            sArr[index] = 0;
          }
          
          messageToken = strtok((char *)&pu8a_server_frame[u16_server_data_off], " ");
          
          for (int index = 0; index < TOKEN_NUM; index++) {
            sArr[index] = messageToken;
            messageToken = strtok(NULL, " ");
          }
          /*
            sArr[0]: verification string
            sArr[1]: index of message
            sArr[2]: time diff in server
            sArr[3]: packet size received
            
          */
          
          /* check correct data */
          if (strcmp(sArr[0],string)>=0 && (atol(sArr[2]) >=  0)) {            
            index_array =  atoi(sArr[1]) % MAXIMUM;
            circular_array[index_array].recvTime =client_recv.tv_nsec ;          
            circular_array[index_array].diff = atol(sArr[2]);            
            diff=circular_array[index_array].recvTime - circular_array[index_array].sendTime;
            
            if (diff < 0) {
              diff = diff + 1000000000;
            }
            
            network_latency = diff - circular_array[index_array].diff;
            
           
            /* print stat */

            if(max_latency < network_latency){
              max_latency=network_latency;
            }
            if(min_latency > network_latency){
              min_latency=network_latency; 
            }
            avg+=network_latency;
            cycle++;
            printf("\tC:%d Min:%ld Act:%ld Avg:%ld Max:%"PRIu64" | Rcv:%d       \r"
                   ,cycle,min_latency,network_latency,(long)avg/cycle,max_latency,atoi(sArr[3]));      
            fflush(stdout);
            
            if(histogram) {
              /* put values in us */
              index_plot= (int)nearbyint(network_latency/1000);
              if(index_plot<MAXPLOT && index_plot>=0){
                array_plot[index_plot]++;              
              }
              else hist_overflow++;            
            }
            
            
            if(cycle >= max_cycle && max_cycle!=0) {
              finish=1; 
              if(histogram==1){
                printf("\n\n"); 
                writeDataForCDFPlot(max_latency,min_latency,avg,cycle,hist_overflow,array_plot); 
                
              } 
              else if(histogram==2){
                printf("\n\n");
                writeDataForPlot(max_latency,min_latency,avg,cycle,hist_overflow,array_plot);  
              } 
              else printf("\n\n"); 
            }
            
            
            
          }
         }	
       }
     
     if(finish)
       return 0;
     
     ///////////////////////////////////////////////////////////////////////////////
}

void * sock_send_synch_thread() {
  //client
  struct sockaddr_ll s_dest_addr;
  int32_t s32_client_sock;
  uint16_t u16_client_data_off;
  uint8_t *pu8a_client_frame;
  uint8_t *pu8a_client_data;
  //end_client
 
  struct sched_attr attr;
  struct timespec client_send,next; 
  struct timespec start;
  struct timespec end;
  uint32_t u32_i = 0;
 
  /*
    Initiate client socket
  */
  if(policy == SCHED_DEADLINE) { 
  attr.size = sizeof(attr);
  attr.sched_flags = 1; // created thread for reception do not take the same scheduler (1)   
  attr.sched_nice = 0;
  attr.sched_priority = 0;
    
  /* creates a 10ms/30ms reservation */
  attr.sched_policy = SCHED_DEADLINE;
  attr.sched_runtime =  run_time * 1000 * 1000;
  attr.sched_period = period * 1000 * 1000;
  attr.sched_deadline = period * 1000 * 1000;
  
  if(sched_setattr(0, &attr, 0) < 0) {
    //done = 0;
    perror("sched_setattr");
    exit(-1);
  }
  }
  
  /* init client frame */
  printf("\nsynch_thread\n");
  u16_client_data_off = (uint16_t) ETH_HLEN;
  pu8a_client_frame   = (uint8_t *) calloc(1, synch_frame_size);
  printf("Data size (CAM) is set to : %d bytes\n", synch_data_size);
  if (pu8a_client_frame == NULL) {
    perror("Could not get memory for the transmit frame\n");
    return 0;
 }
  
  pu8a_client_data    = pu8a_client_frame + u16_client_data_off;
  
 /* set client frame header */
  (void)memcpy(pu8a_client_frame, get_dest_addr() , ETH_ALEN);
  (void)memcpy(pu8a_client_frame + ETH_ALEN, get_src_addr() , ETH_ALEN);
  
  
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
 printf("Send Socket created\n");
  
 /* set client sockaddr_ll */
  
  s_dest_addr.sll_family = AF_PACKET;
  /*we don't use a protocol above ethernet layer, just use anything here*/
  s_dest_addr.sll_protocol = htons(ETH_P_ALL);
  s_dest_addr.sll_ifindex = get_nic_index(get_nic_name());
  s_dest_addr.sll_hatype = ARPHRD_ETHER;
  s_dest_addr.sll_pkttype = PACKET_OTHERHOST; //PACKET_OUTGOING
  s_dest_addr.sll_halen = ETH_ALEN;
  /*MAC - begin*/
  s_dest_addr.sll_addr[0] = get_dest_addr()[0];
  s_dest_addr.sll_addr[1] = get_dest_addr()[1];
  s_dest_addr.sll_addr[2] = get_dest_addr()[2];
  s_dest_addr.sll_addr[3] = get_dest_addr()[3];
  s_dest_addr.sll_addr[4] = get_dest_addr()[4];
  s_dest_addr.sll_addr[5] = get_dest_addr()[5];
  /*MAC - end*/
  s_dest_addr.sll_addr[6] = 0x00; /*not used*/
  s_dest_addr.sll_addr[7] = 0x00; /*not used*/
  
  /*
    Create pthread for receiving the message
  */
  


  printf("Sending data using raw socket over  %s \n\n", get_nic_name());
  
  clock_gettime(CLOCK_MONOTONIC, &next);
  
  /////////////////////////////////////////////////////////////////// 
  while (1)
    {
      /*
	Sending the message to the server
      */
      
      /* init_client_data */
      clock_gettime(CLOCK_MONOTONIC, &start); // for EDF
      (void)memset(&pu8a_client_frame[u16_client_data_off], '\0',synch_data_size);     
      clock_gettime(CLOCK_MONOTONIC_RAW, &client_send);
      
      /* putSendTime */
      int indexOfArray = u32_i % MAXIMUM;
      circular_array[indexOfArray].sendTime = client_send.tv_nsec;
      
      (void)snprintf((char *)&pu8a_client_frame[u16_client_data_off],
                     synch_data_size ,"%s %d %ld %d","synch_thread" , u32_i++ ,client_send.tv_nsec, 0);
      
      /* send client data */
      int send_result = sendto(s32_client_sock,
                               pu8a_client_frame,
                               synch_frame_size,
                               0,
                               (struct sockaddr *)&s_dest_addr,
                               sizeof(s_dest_addr));
      
      
      if (send_result == -1) {
        perror("Socket send failed");
        free(pu8a_client_frame);
        pu8a_client_frame = NULL;
        pu8a_client_data  = NULL;
        if (s32_client_sock > 0) {
          close(s32_client_sock);
        }
        
        return 0;
      }            
                
      // Normalize the time to account for the second boundary
      if(policy!= SCHED_DEADLINE) {
        if(next.tv_nsec > (1000000000 - period)) {
          next.tv_nsec += period - 1000000000 ;
          next.tv_sec++;
        }else next.tv_nsec += period;
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL);
      }
      
      else {
        clock_gettime(CLOCK_MONOTONIC, &end);
        if(fn_diff(start,end).tv_nsec > attr.sched_deadline)
          printf(" Missed Deadline \n");
        while(fn_diff(start,end).tv_nsec <= attr.sched_runtime)
          {
            clock_gettime(CLOCK_MONOTONIC, &end);
          }
      }
      
      
      if(finish) {   
        return 0;
      }
    }
}

void * sock_send_asynch_thread() {
  //client
  struct sockaddr_ll s_dest_addr;
  int32_t s32_client_sock;
  uint16_t u16_client_data_off;
  uint8_t *pu8a_client_frame;
   uint8_t *pu8a_client_data;
  //end_client
  
  struct sched_attr attr;
  struct timespec client_send,next;
  struct timespec start;
  struct timespec end;
  uint32_t u32_i = 0;
  int rand_period = 0 ;

 /*
    Initiate client socket
 */
  
  printf("asynch_thread\n");
  /* init client frame */  
  u16_client_data_off = (uint16_t) ETH_HLEN;
  pu8a_client_frame   = (uint8_t *) calloc(1, asynch_frame_size);
  printf("Data size (DENM) is set to : %d bytes\n", asynch_data_size);
  
  if (pu8a_client_frame == NULL) {
    perror("Could not get memory for the transmit frame\n");
    return 0;
  }
  
  pu8a_client_data    = pu8a_client_frame + u16_client_data_off;
  
  /* set client frame header */
  (void)memcpy(pu8a_client_frame, get_dest_addr() , ETH_ALEN);
  (void)memcpy(pu8a_client_frame + ETH_ALEN, get_src_addr() , ETH_ALEN);
  
  
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
  printf("Send Socket created\n");
  
  /* set client sockaddr_ll */
  
  s_dest_addr.sll_family = AF_PACKET;
  /*we don't use a protocol above ethernet layer, just use anything here*/
  s_dest_addr.sll_protocol = htons(ETH_P_ALL);
  s_dest_addr.sll_ifindex = get_nic_index(get_nic_name());
  s_dest_addr.sll_hatype = ARPHRD_ETHER;
  s_dest_addr.sll_pkttype = PACKET_OTHERHOST; //PACKET_OUTGOING
  s_dest_addr.sll_halen = ETH_ALEN;
  /*MAC - begin*/
  s_dest_addr.sll_addr[0] = get_dest_addr()[0];
  s_dest_addr.sll_addr[1] = get_dest_addr()[1];
  s_dest_addr.sll_addr[2] = get_dest_addr()[2];
  s_dest_addr.sll_addr[3] = get_dest_addr()[3];
  s_dest_addr.sll_addr[4] = get_dest_addr()[4];
  s_dest_addr.sll_addr[5] = get_dest_addr()[5];
  /*MAC - end*/
  s_dest_addr.sll_addr[6] = 0x00; /*not used*/
  s_dest_addr.sll_addr[7] = 0x00; /*not used*/
  
  /*
    Create pthread for receiving the message
  */
  


  printf("Sending data using raw socket over  %s \n\n", get_nic_name());
  
  clock_gettime(CLOCK_MONOTONIC, &next);
  
  /////////////////////////////////////////////////////////////////// 
  while (1)
    {
      /*
	Sending the message to the server
      */
      
      /* init_client_data */
      clock_gettime(CLOCK_MONOTONIC, &start); // for EDF
      (void)memset(&pu8a_client_frame[u16_client_data_off], '\0',asynch_data_size);     
      clock_gettime(CLOCK_MONOTONIC_RAW, &client_send);
      
      /* putSendTime */
      int indexOfArray = u32_i % MAXIMUM;
      circular_array[indexOfArray].sendTime = client_send.tv_nsec;
      
      (void)snprintf((char *)&pu8a_client_frame[u16_client_data_off],
                     asynch_data_size ,"%s %d %ld %d","asynch_thread" , u32_i++ ,client_send.tv_nsec, 0);
      
      /* send client data */
      int send_result = sendto(s32_client_sock,
                               pu8a_client_frame,
                               asynch_frame_size,
                               0,
                               (struct sockaddr *)&s_dest_addr,
                               sizeof(s_dest_addr));
      
      
      if (send_result == -1) {
        perror("Socket send failed");
        free(pu8a_client_frame);
        pu8a_client_frame = NULL;
        pu8a_client_data  = NULL;
        if (s32_client_sock > 0) {
          close(s32_client_sock);
        }
        return 0;
      }
      
      rand_period= my_rand(); // between 5ms and 20ms 
    
        if(next.tv_nsec > (1000000000 - rand_period)) {
          next.tv_nsec += rand_period - 1000000000 ;
          next.tv_sec++;
        }else next.tv_nsec += rand_period;
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL);
      
      
      if(finish) {
        free(pu8a_client_frame);
        pu8a_client_frame = NULL;
        pu8a_client_data  = NULL;
        if (s32_client_sock > 0) {
          close(s32_client_sock);
        }
        free_server_frame();
        close_server_socket();
        return 0;
      }
     
    }
}
