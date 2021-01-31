#include "control_server.h"

int init_server_frame(int frame_size) {
    u16_server_data_off = (uint16_t) ETH_HLEN;
    pu8a_server_frame   = (uint8_t *) calloc(1, frame_size);

    if (pu8a_server_frame == NULL) {
        perror("Could not get memory for the transmit frame\n");
        return ERROR_CREATE_FRAME;
    }

    pu8a_server_data    = pu8a_server_frame + u16_server_data_off;

    return NO_ERROR;
}

void free_server_frame() {
    free(pu8a_server_frame);
    pu8a_server_frame = NULL;
    pu8a_server_data  = NULL;
}

void init_server_socket() {
    s32_server_sock = -1;
}

int set_server_socket() {
    s32_server_sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

    if (s32_server_sock == -1) {
        perror("Could not create the socket");

        return ERROR_CREATE_SOCKET;
    }

    printf("Receive Socket created\n");
    return NO_ERROR;
}

int bind_server_socket() {
    int resultFunction = bind(s32_server_sock,
                   (struct sockaddr *)&s_src_addr,
                   sizeof(s_src_addr));

    if (resultFunction == -1) {
        perror("Could not bind to the socket");

        return ERROR_BIND_SOCKET;
    }

    return NO_ERROR;
}

void close_server_socket() {
    if (s32_server_sock > 0) {
        close(s32_server_sock);
    }
}

void init_server_sockaddr_ll() {
    (void)memset(&s_src_addr, 0, sizeof(s_src_addr));
}

void set_server_sockaddr_ll(int32_t nic_index) {
    s_src_addr.sll_family = AF_PACKET;
    /*we don't use a protocol above ethernet layer, just use anything here*/
    s_src_addr.sll_protocol = htons(ETH_P_ALL);
    s_src_addr.sll_ifindex = nic_index;
    s_src_addr.sll_hatype = ARPHRD_ETHER;
    s_src_addr.sll_pkttype = PACKET_OTHERHOST; //PACKET_OUTGOING
    s_src_addr.sll_halen = ETH_ALEN;

    socklen_t u32_src_addr_len = sizeof(s_src_addr);
}

int receive_data(int frame_size) {
    int resultFunction = recvfrom(s32_server_sock,
                           pu8a_server_frame,
                           frame_size,
                           0,
                           (struct sockaddr *)&s_src_addr,
                           &u32_src_addr_len);

    if (resultFunction == -1) {
        perror("Socket receive failed");

        return FAIL_RECEIVE_DATA;
    }

    else if (resultFunction < 0) {
        perror("Socket receive, error ");

        return ERROR_RECEIVE_DATA;
    }

    return NO_ERROR;
}

int check_data_from_target(uint8_t* target_address) {
    for (int index = 0; index < sizeof(s_src_addr.sll_addr) - 2; index++) {
        if (s_src_addr.sll_addr[index] != target_address[index]) {
/*
This command print the entire message when the server receive unexpected message
But during test, it will get look back messages
*/

//                printf("  Received by unexpected target : %s\n", pu8a_server_data);

                return NOT_FROM_TARGET;
            }
    }

    return FROM_TARGET;
}

void parse_data() {
    char* messageToken = NULL;

    for (int index = 0; index < TOKEN_NUM; index++) {
        sArr[index] = 0;
    }
    
    messageToken = strtok(&pu8a_server_frame[u16_server_data_off], " ");

    for (int index = 0; index < TOKEN_NUM; index++) {
        sArr[index] = messageToken;
        messageToken = strtok(NULL, " ");
    }
}

int check_correct_data(char* string) {
    if (strcmp(get_packet_string(),string) || atol(get_packet_timestamp()) < 0) {
        return NOT_CORRECT_DATA;
    }

    return CORRECT_DATA;
}

char* get_packet_string() {
    return sArr[0];
}

char* get_packet_index() {
    return sArr[1];
}

char* get_packet_timestamp() {
    return sArr[2];
}

void print_target_mac_addr() {
    printf("  Received data from ");

    for (int index = 0; index < sizeof(s_src_addr.sll_addr) - 2; index++) {
        printf("%02x:", s_src_addr.sll_addr[index]);
    }

    printf("\n");
}

void print_packet_string() {
    printf("  %s ", get_packet_string());
}

void print_packet_index() {
    printf("%s ", get_packet_index());
}

void print_packet_timestamp() {
    printf("msg sent at %s (ns)\r", get_packet_timestamp());
     fflush(stdout);
}
