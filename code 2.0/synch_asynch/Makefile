all: client server

server: server.o get_nic_index.o write_data.o get_configuration.o 
	gcc -std=c99 -g -Wall -pthread -o server server.o get_nic_index.o write_data.o -lm -lncurses get_configuration.o

client: client.o get_nic_index.o write_data.o get_configuration.o 
	gcc -std=c99 -g -Wall -pthread -o client client.o get_nic_index.o write_data.o -lm -lncurses get_configuration.o

client.o: get_nic_index.h write_data.h get_configuration.h client.c
	gcc -pthread -c -std=c99 -g -Wall -o client.o client.c

server.o: get_nic_index.h get_configuration.h server.c
	gcc -pthread -c -std=c99 -g -Wall -o server.o server.c

write_data.o: write_data.h write_data.c
	gcc -c -std=c99 -g -Wall -o write_data.o write_data.c

get_configuration.o: get_configuration.h get_configuration.c
	gcc -c -std=c99 -g -Wall -o get_configuration.o get_configuration.c

get_nic_index.o: get_nic_index.h get_nic_index.c
	gcc -c -std=c99 -g -Wall -o get_nic_index.o get_nic_index.c

clean:
	rm -f *.o client server
#gcc -std=c99 -g -Wall source.o -o file
#gcc -c -std=c99 -g -Wall source.c
