/*====================================================================================
 Final project in Lab session of CCS2 course.
 Written by Ilan Klein & Shir Granit.
 
				
				----------  Ascii-Film App:  Server side  ----------
	
	
	compile with gcc server.c -o film_server
	run with ./film_server <tcp control port> <tcp pro port> <multicast ip adr>
							<udp port> <file1> <file2> <file3> <file4> ...
  ==================================================================================*/
  
//--Includes:

//--Defines:
#define client_max_size 50
#define client_pro_size 2

//--Global variables
//Array of clients tracker = size clients_max_size
//Pro users tracker = size client_pro_size
//

//--Help functions
//transsmit new frame in Transsmiting movies section


////~////~//// MAIN ////~////~////

//--declare main variables

//--Decode to variables input from command line
//declare an array of bits for movies transmission 

//--Open a Welcome socket (TCP control with clients)


//-------------------------------------------------------------------
//--Open a new thread: Transsmiting movies
//use the select function to know which movie to switch frame

//-------------------------------------------------------------------
//--Open a new thread: listening for new clients
//listening
//print "ready for clients"

//receive hello
//--if a new client came -> open a new child process for client communication
//send welcome (hello response)
//establish a connection ++ Timeout
//proper responses for requests: get film, change channel, pro, speed up, release...


//--if a new client came -> father process go back to listen



// @ Ilan & Shir.