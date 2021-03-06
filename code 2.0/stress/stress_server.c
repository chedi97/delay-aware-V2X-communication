/*
 Coded by Hyunjae Lee and Jihun Lim
 
 This is a simple implementation for latency evaluation in a V2V network.
 We assume that all node knows other MAC addresses who want to communicate with

 [Server]
 This is a receiver that is waiting until it gets a message with link-layer header from the client.
 Whenever the receiver gets a message from clients,
 the receiver shows user link-layer header and payload.

  *Reference : https://stackoverflow.com/questions/10824827/raw-sockets-com$
*/
#define _GNU_SOURCE
#include <sched.h>
#include <getopt.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "result_structure.h"
#include "get_nic_index.h"
#include "get_configuration.h"
#include "packet_size.h"
#include "control_server.h"
#include "control_client.h"

#define DEFAULT_RTPRIORITY 49 

int policy = SCHED_OTHER;	/* default policy if not specified */
int priority=0;
int main_affinity=0;


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
     case 's': set_packet_size(optarg);break;
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


int main(int argc, char *argv[])
{
    struct timespec server_send, server_recv;
    struct sched_param schedp;

/*
Initiate other modules : get_configuration.c , packet_size.c
*/
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
Initiate server socket
*/
    if (init_server_frame(get_packet_size(FRAME)) != NO_ERROR) {
        return 0;
    }

    init_server_socket();
    init_server_sockaddr_ll();
    set_server_sockaddr_ll(get_nic_index(get_nic_name()));

    if (set_server_socket() != NO_ERROR) {
        free_server_frame();
        return 0;
    }

    if (bind_server_socket() != NO_ERROR) {
        free_server_frame();
        close_server_socket();
        return 0;
    }



/*
Initiate client socket
*/
    if(init_client_frame(get_packet_size(FRAME)) != NO_ERROR) {
        free_server_frame();
        close_server_socket();
        return 0;
    }

    set_client_frame_header(get_dest_addr(), get_src_addr());

    init_client_socket();
    init_client_sockaddr_ll();

    if (set_client_socket() != NO_ERROR) {
        free_server_frame();
        close_server_socket();
        free_client_frame();
        return 0;
    }

    set_client_sockaddr_ll(get_nic_index(get_nic_name()), get_src_addr());



    while (1)
    {
/*
Receive the message from the target
*/
        init_server_sockaddr_ll();

        if (receive_data(get_packet_size(FRAME)) != NO_ERROR) {
            free_server_frame();
            close_server_socket();
            free_client_frame();
            close_client_socket();
            return 0;
        }

        clock_gettime(CLOCK_MONOTONIC_RAW, &server_recv);
        if (check_data_from_target(get_src_addr()) == FROM_TARGET) {
            parse_data();

            //if (check_correct_data("raw_packet_test") == CORRECT_DATA) {
	    //  //print_target_mac_addr();
	    //  // print_packet_string();
            //  //  print_packet_index();
            //    print_packet_timestamp();


/*
Send the message back to the target
*/
              /*  init_client_data(get_packet_size(DATA));
                clock_gettime(CLOCK_MONOTONIC_RAW, &server_send);
                set_client_data(get_packet_size(DATA), "Index/Diff", \
                atoi(get_packet_index()), \
                timespec_diff(server_recv.tv_nsec, server_send.tv_nsec));
			
                if (send_client_data(get_packet_size(FRAME)) != NO_ERROR) {
                    free_server_frame();
                    close_server_socket();
                    free_client_frame();
                    close_client_socket();

                    return 0;
                }
	      */
		
              //  printf("==============================================\n");
            //}
        }
    }

    return 0;
}
