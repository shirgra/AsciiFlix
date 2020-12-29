/*================================================================
UDP sender:
	copile with gcc UDP_sender.c -o UDP_sender
	run with ./UDP_sender 234.0.0.0 5555 num_parts alice.txt
  ================================================================*/

//includes
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h> //read files

//defines
#define buffer_size 1024

//main - UDP sender:
int main(int argc, char** argv)
{
	//help variables:
	int a , b , c , l , r , fd;
	int sent_byte_num , expected_byte_num;
	int myUDPsocket;
	int port_num, num_parts;
	char buf[buffer_size];//buffer for data
	ssize_t write_to_socket;//read from socket	
	struct sockaddr_in sock_struct;//creat sockaddr struct:
	struct ip_mreq mreq;//create multicast request

	//input vars - convert to usefull vars:
	//ip_addr_sender
	char * str_mc_addr = argv[1];
	in_addr_t mc_addr = inet_addr(str_mc_addr);
	//port number
	char * str_port = argv[2];
	port_num = atoi(str_port);
	//num_parts - each block 1024 bytes
	char * str_parts = argv[3];
	num_parts = atoi(str_parts);
	//file_name - name of the file to transsfer to reciever
	char * file_name = argv[4];

	//byte num:
	expected_byte_num = num_parts * buffer_size;
	sent_byte_num = 0 ;

	//prepare txt file
	fd = open( file_name , O_RDONLY );
	if(fd==-1){ printf("ERR- can not open .txt file\n"); return -1; } 

	/*SOCKET PROCESS*/
	
	//open a UDP socket
	myUDPsocket = socket(AF_INET , SOCK_DGRAM , 0);
	
	//set socket properties
	sock_struct.sin_family = AF_INET;
	sock_struct.sin_port = htons(port_num);
	sock_struct.sin_addr.s_addr = inet_addr(str_mc_addr);

	//multicast definitions
	u_char ttl;
	ttl = 32;
	setsockopt( myUDPsocket , IPPROTO_IP , IP_MULTICAST_TTL ,&ttl , sizeof(ttl));
	
	//while sent_byte_num didnt reach expected_byte_num
	do
	{
		//read file.txt into buffer
		r = read ( fd , buf , buffer_size );
		
		//write to socket
		write_to_socket = sendto( myUDPsocket , buf , buffer_size , 0 , (struct sockaddr *) &sock_struct , sizeof(sock_struct)) ;
		
		//update actual_byte_num
		sent_byte_num = sent_byte_num + (int)write_to_socket ;
		
	}//end while
	while( sent_byte_num < expected_byte_num );
		
		
	//print number of bytes sent
	printf("Sent packets.\nActual amount of bytes sent is %d.\n", sent_byte_num);

	//close socket
	//setsockopt( myUDPsocket , IPPROTO_IP , IP_DROP_MEMBERSHIP , &mreq , sizeof(mreq));
	close(myUDPsocket);
	close(fd);
	
	return 0;

		
}//end main
