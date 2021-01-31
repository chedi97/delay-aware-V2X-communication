/*
Coded by Hyunjae Lee and Jihun Lim

This header file is for saving and calculating the latency

In this header, There is the Array to save the value to container
Each index is consist of 3 part

sendTime : the time that the client send the message to the server
recvTime : the time that the client receive the message from the server
diff     : the time from the server receive the message from the client to the server send the message back to the client

Each put functions can put the value to the proper index's place
The index will be calculated as index % MAXIMUM to avoid outbound error

Each get functions can get the value from the index's place

MessageLatency will be calculated as recvTime - sendTime
NetworkLatency will be calculated as recvTime - sendTime - diff

If the array is full, isFull function will return FULL flag

            (index % MAXIMUM)
                v
                0        1       2       3       4       5       6       ...
            =========================================================
sendTime    |       |       |       |       |       |       |       |     
            =========================================================
recvTime    |       |       |       |       |       |       |       |
            =========================================================
diff        |       |       |       |       |       |       |       |
            =========================================================
*/

#include <time.h>
#include <math.h>
#include "common_header.h"

#define MAXIMUM 200

#define FULL 1
#define NOT_FULL 0


struct timeGap
{
    long sendTime;
    long recvTime;
    long diff;
};

struct timeGap circular_array[MAXIMUM];


void initArray();
int isFull();

void putSendTime(long sendTime, int index);
void putRecvTime(long recvTime, int index);
void putDiff(long diff, int index);

long getSendTime(int index);
long getRecvTime(int index);
long getDiff(int index);

long getMessageLatency(int index);
long getNetworkLatency(int index);

void printData(int index);

long timespec_diff(long start, long stop);


