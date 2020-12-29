/*================================================================
UDP receiver:
	copile with gcc UDP_receiver.c -o UDP_receiver
	run with ./UDP_receiver 234.0.0.0 5555 num_parts
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

//main - UDP receiver:
int main(int argc, char** argv)
{
	//variables:
	int b;
	int myUDPsocket;
	int expected_byte_num;
	int actual_byte_num;
	char buf[buffer_size];//buffer for data
	ssize_t read_from_socket;//read from socket
	struct ip_mreq mreq;//create multicast request
	struct sockaddr_in sock_struct;//create sockaddr struct:

	//input vars - convert to usefull vars:
	//ip_addr_sender
	char * str_mc_addr = argv[1];
	in_addr_t mc_addr = inet_addr(str_mc_addr);
	//port number
	char * str_port = argv[2];
	int port_num = atoi(str_port);
	//num_parts - each block 1024 bytes
	char * str_parts = argv[3];
	int num_parts = atoi(str_parts);

	//byte num:
	expected_byte_num = buffer_size * num_parts ;
	actual_byte_num = 0 ;

	//open a UDP socket
	myUDPsocket = socket(AF_INET , SOCK_DGRAM , 0);
	
	//set socket properties
	sock_struct.sin_family = AF_INET ;
	sock_struct.sin_port = htons(port_num) ;
	sock_struct.sin_addr.s_addr = htonl(INADDR_ANY);
	
	//bind
	b = bind( myUDPsocket , (struct sockaddr *) &sock_struct , sizeof(sock_struct) );
	
	//connect to a multicast group
	mreq.imr_multiaddr.s_addr = mc_addr;
	mreq.imr_interface.s_addr = htonl(INADDR_ANY);
	
	//connect to m.c. group
	printf("UDP listener is waiting...\n");
	setsockopt( myUDPsocket , IPPROTO_IP , IP_ADD_MEMBERSHIP , &mreq , sizeof(mreq));
	
	int s;
	s= sizeof(sock_struct);

	//while sent_byte_num didnt reach expected_byte_num
	do
	{
		//read socket data into buffer
		read_from_socket = recvfrom ( myUDPsocket , buf , buffer_size , 0 , (struct sockaddr *) &sock_struct , &s);
		
		//read socket data into buffer & write to file / print to screen
		printf("%s", buf);

		//update actual_byte_num and iterator
		actual_byte_num = actual_byte_num + read_from_socket ;

	}//end while
	while( actual_byte_num < expected_byte_num );
		
	//print number of bytes we needed to receive & actual amount received
	printf("\n\nSupposed to receive %d bytes. Actual amount received is %d bytes.\n", expected_byte_num , actual_byte_num);
	printf("Finished receiving\n");
	
	//free al resorces
	setsockopt( myUDPsocket , IPPROTO_IP , IP_DROP_MEMBERSHIP , &mreq , sizeof(mreq));
	close(myUDPsocket);
	return 0;
}//end main
