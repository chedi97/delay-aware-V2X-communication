#include "get_configuration.h"

int get_configuration() {
    FILE *pFile = NULL;
    pFile = fopen("configuration.txt", "r");

    int indexForLine = 0;

    if (pFile != NULL) {
        char saveOneLine[255];
        char* pointerForToken = NULL;

        while (!feof(pFile)) {
            fgets(saveOneLine, sizeof(saveOneLine), pFile);
            pointerForToken = strtok(saveOneLine, ":");

            if(indexForLine == 0) {
                pointerForToken = strtok(NULL, "\n");
                

                NIC_NAME = (char*)malloc(sizeof(char)*strlen(pointerForToken));
                strcpy(NIC_NAME, pointerForToken);
            }

            else if(indexForLine == 1) {
                for(int index = 0; index < MAC_ADDR_LEN; index++) {
                    pointerForToken = strtok(NULL, ",");

                    gu8a_src_mac[index] = (uint8_t)strtol(pointerForToken, NULL, 16);

                    if(gu8a_src_mac[index] < 0x00 || gu8a_src_mac[index] > 0xff) {
                        perror("ERROR_WRONG_MAC : check your mac address in configuration file\n");

                        fclose(pFile);
                        return ERROR_WRONG_MAC;
                    }
                }
            }

            else if(indexForLine == 2) {
                for(int index = 0; index < MAC_ADDR_LEN; index++) {
                    pointerForToken = strtok(NULL, ",");

                    gu8a_dest_mac[index] = (uint8_t)strtol(pointerForToken, NULL, 16);

                    if(gu8a_dest_mac[index] < 0x00 || gu8a_dest_mac[index] > 0xff) {
                        perror("ERROR_WRONG_MAC : check your mac address in configuration file\n");

                        fclose(pFile);
                        return ERROR_WRONG_MAC;
                    }
                }
            }

            else if(indexForLine == 3) {
                pointerForToken = strtok(NULL, "\n");
                frequency = atoi(pointerForToken);

                if(frequency < 0 || frequency > 10000) {
                    perror("ERROR_WRONG_FRE : check your frequency in configuration file\n");

                    fclose(pFile);
                    return ERROR_WRONG_FRE;
                }
            }

            else if(indexForLine == 4) {
                pointerForToken = strtok(NULL, "\n");
                bandwidth = atoi(pointerForToken);

                if (bandwidth < 0 || bandwidth > 300) {
                    perror("ERROR_WRONG_BAND : check your bandwidth in configuration file\n");

                    fclose(pFile);
                    return ERROR_WRONG_BAND;
                }
            }

            indexForLine++;
        }
    }

    else {
        perror("ERROR_OPEN_FILE : check configuration file in your code folder\n");

        fclose(pFile);
        return ERROR_OPEN_FILE;
    }

    fclose(pFile);
    return NO_ERROR;
}

char* get_nic_name() {
    return NIC_NAME;
}

uint8_t* get_src_addr() {
    return gu8a_src_mac;
}

uint8_t* get_dest_addr() {
    return gu8a_dest_mac;
}

int get_frequency() {
    return frequency;
}

int get_bandwitch() {
    return bandwidth;
}