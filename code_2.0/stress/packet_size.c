#include "packet_size.h"

int set_packet_size(char *packet_size)
{
  /*  if (strcmp(packet_size, "LARGE") == 0)
    {
        frame_size = ETH_FRAME_LEN;
        data_size = ETH_FRAME_LEN - ETH_HLEN;
    }

    else if (strcmp(packet_size, "SMALL") == 0)
    {
        frame_size = ETH_ZLEN;
        data_size = ETH_ZLEN - ETH_HLEN;
	}*/
  if(atoi(packet_size)>=ETH_ZLEN && atoi(packet_size)<=ETH_FRAME_LEN){
    frame_size =atoi(packet_size) ;
    data_size = atoi(packet_size) - ETH_HLEN;
     }
  
   else
    {
        perror("You need to put packet size between 60 and 1514 \n");
        //return ERROR_WORD;
	 exit(EXIT_FAILURE);
	  exit(EXIT_SUCCESS);
    }

  //return NO_ERROR;

}

int get_packet_size(int option)
{
    if (option == FRAME) // frame
    {
        return frame_size;
    }
    else if (option == DATA) //data
    {
        return data_size;
    }
}
