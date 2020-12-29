/*================================================================
TCP sender:
	copile with gcc TCP_sender.c -o TCP_sender
	make sure alice.txt is in the same folder
	run with ./TCP_sender 192.10.src.1 5555 num_parts alice.txt
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

//main - TCP sender:
int main(int argc, char** argv)
{
	//help variables:
	int a , b , c , l , r , fd;
	int sent_byte_num;
	int welcomeTCPsocket;
	int port_num, num_parts;
	char buf[buffer_size];//buffer for data
	ssize_t write_to_socket;//read from socket	
	struct sockaddr_in sock_struct;//creat sockaddr struct:
	struct sockaddr_in client;//socket for client

	//input vars - convert to usefull vars:
	//ip_addr_sender
	char * str_ip_addr_sender = argv[1];
	in_addr_t ip_addr_sender = inet_addr(str_ip_addr_sender);
	//port number
	char * str_port = argv[2];
	port_num = atoi(str_port);
	//num_parts - each block 1024 bytes
	char * str_parts = argv[3];
	num_parts = atoi(str_parts);
	//file_name - name of the file to transsfer to reciever
	char * file_name = argv[4];

	//byte num:
	sent_byte_num = 0 ;

	//prepare txt file
	fd = open( file_name , O_RDONLY );
	if(fd==-1){ printf("ERR- can not open .txt file\n"); return -1; } 

	/*SOCKET PROCESS*/
	
	//open a TCP socket
	welcomeTCPsocket = socket(AF_INET , SOCK_STREAM , 0);

	//set vars to sock struct - srv:
	sock_struct.sin_family = AF_INET ;
	sock_struct.sin_port = htons(port_num) ;
	sock_struct.sin_addr.s_addr = inet_addr(str_ip_addr_sender);

	//bind socket
	b = bind( welcomeTCPsocket , (struct sockaddr *) &sock_struct, sizeof(sock_struct));
	
	//wait for a connection - listen
	l = listen(welcomeTCPsocket, SOMAXCONN);
	//listening...
	if(!l){	printf("Listening...\n"); }
	else{ printf("ERR\n"); return -1; }

	//accept connection from an incoming client
	c = sizeof(struct sockaddr_in);
	a = accept(welcomeTCPsocket, (struct sockaddr *)&client, (socklen_t*)&c);

	//while: not all data sent OR connection error
	do
	{
		//read file.txt into buffer
		r = read ( fd , buf , buffer_size );
		//write to socket
		write_to_socket = send( a , buf , buffer_size , 0 );
		//update actual_byte_num
		sent_byte_num = sent_byte_num + (int)write_to_socket ;
	}
	while( (sent_byte_num < (num_parts*buffer_size)) || (write_to_socket <= 0) );

	//print number of bytes sent
	printf("Sent packets.\nActual amount of bytes sent is %d.\n", sent_byte_num);

	//close socket
	close(welcomeTCPsocket);
	close(fd);
	
	return 0;
}//end main
