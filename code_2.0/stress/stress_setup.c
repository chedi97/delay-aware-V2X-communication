/*
Coded by Hyunjae Lee and Jihun Lim

This is setting program for OCB mode

this program can get the values in configuration.txt
and set the wireless hardware driver using the values

this program give some commands to bash program
and the bash will execute those commands

in configuration.txt file, there are three arguments for this setting
NIC_NAME : save the name of hardware driver we want to use
BANDWIDTH : save the value of bandwidth
FREQUENCY : save the value of frequency

the command is consist of below,

ip link set NIC_NAME down
iw dev NIC_NAME set type ocb
ip link set NIC_NAME up
iw dev NIC_NAME ocb join BANDWIDTH FREQUENCY MHZ
iw dev NIC_NAME info
*/

#include "common_header.h"
#include "get_configuration.h"

int main()
{
	/*
	command[255] : save the command
	get_configuration() : from configuration.txt, save the values to the variables
	*/

    char command[255];
    get_configuration();

	/*
	this part is executing the bash command in bash
	using system function, we can give the commands to bash
	
	the command is consist of below,

	ip link set NIC_NAME down							// shut down the hardware driver
	iw dev NIC_NAME set type ocb						// set the type to OCB mode
	ip link set NIC_NAME up								// wake up the hardware driver
	iw dev NIC_NAME ocb join BANDWIDTH FREQUENCY MHZ	// set the bandwidth and frequency values for hardware driver
	iw dev NIC_NAME info								// show the information of the hardware driver
	*/

    sprintf(command, "/bin/bash -c 'ip link set %s up'", get_nic_name());
    system(command);
    //sprintf(command, "/bin/bash -c 'ip addr add 192.168.2.2 dev %s'", get_nic_name()); //
    // system(command);
    // sprintf(command, "/bin/bash -c 'iw dev %s set type ocb'", get_nic_name());
    //system(command);
    // sprintf(command, "/bin/bash -c 'ip link set %s up'", get_nic_name());
    //system(command);
    //sprintf(command, "/bin/bash -c 'iw dev %s ocb join %d %dMHZ'", get_nic_name(), get_frequency(), get_bandwitch());
    //system(command);
    // sprintf(command, "/bin/bash -c 'iw dev %s info'", get_nic_name());
    //system(command);

    return 0;
}
