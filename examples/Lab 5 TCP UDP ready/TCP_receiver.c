/*================================================================
TCP client:
	copile with gcc TCP_receiver.c -o TCP_receiver
	run with ./TCP_receiver 192.10.src.1 5555 num_parts
  ================================================================*/

//includes
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//defines
#define buffer_size 1024

//main - TCP receiver:
int main(int argc, char** argv)
{
	//variables:
	int expected_byte_num;
	int actual_byte_num;
	char buf[buffer_size];//buffer for data
	ssize_t read_from_socket;//read from socket

	//input vars - convert to usefull vars:
	//ip_addr_sender
	char * str_ip_addr_sender = argv[1];
	in_addr_t ip_addr_sender = inet_addr(str_ip_addr_sender);
	//port number
	char * str_port = argv[2];
	int port_num = atoi(str_port);
	//num_parts - each block 1024 bytes
	char * str_parts = argv[3];
	int num_parts = atoi(str_parts);

	//byte num:
	expected_byte_num = buffer_size * num_parts ;
	actual_byte_num = 0 ;
	
	//creat sockaddr struct:
	struct sockaddr_in sock_struct;
	//set vars:
	sock_struct.sin_family = AF_INET ;
	sock_struct.sin_port = htons(port_num) ;
	sock_struct.sin_addr.s_addr = inet_addr(str_ip_addr_sender);

	/*SOCKET PROCESS*/
	
	// open a TCP socket
	int myTCPsocket = socket(AF_INET , SOCK_STREAM , 0);
	
	//connect to sender
	int c = connect( myTCPsocket , (struct sockaddr*) &sock_struct , sizeof(sock_struct) );
	if(c == -1){c = connect( myTCPsocket , (struct sockaddr*) &sock_struct , sizeof(sock_struct) );}
	
	//while: not all data received OR connection closed by sender
	do
	{
		//read from socket
		read_from_socket = recv ( myTCPsocket , buf , buffer_size , 0 );
		
		//read socket data into buffer & write to file / print to screen
		printf("%s", buf);

		//update actual_byte_num
		actual_byte_num = actual_byte_num + read_from_socket ;
	}
	while( read_from_socket > 0 );
		
	//print number of bytes we needed to receive & actual number of bites actual received
	printf("\n\nSupposed to receive %d bytes. Actual amount received is %d bytes.\n", expected_byte_num , actual_byte_num);
	printf("Finished receiving\n");

	//close socket
	close(myTCPsocket);
	return 0;
}//end main
