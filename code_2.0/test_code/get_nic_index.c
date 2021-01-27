#include "get_nic_index.h"

/* NIC Index : unique identifying number for a physical or logical network device */

int32_t get_nic_index(char *nic_card_name)
{
    int32_t s32_sock_fd = -1;
    int32_t s32_res = -1;
    struct ifreq s_ifr;

    (void)memset(&s_ifr, 0, sizeof(s_ifr));

    s32_sock_fd = socket(AF_INET, SOCK_DGRAM, 0);

    if (-1 == s32_sock_fd)
    {
        perror("get_nic_index(): socket failed");
        
        return ERROR_CREATE_SOCKET;
    }

    s_ifr.ifr_addr.sa_family = AF_INET;

    strncpy(s_ifr.ifr_name, nic_card_name, IFNAMSIZ);

    //printf("NIC : %s\n", nic_card_name);
    s32_res = ioctl(s32_sock_fd, SIOCGIFINDEX, &s_ifr);

    close(s32_sock_fd);

    if (-1 == s32_res)
    {
        perror("get_nic_index(): ioctl failed");

        return ERROR_IOCTL;
    }

    //printf("NIC INDEX : %d\n", s_ifr.ifr_ifru.ifru_ivalue);
    return (s_ifr.ifr_ifru.ifru_ivalue);
}
