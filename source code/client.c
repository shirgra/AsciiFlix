/*====================================================================================
 Final project in Lab session of CCS2 course.
 Written by Ilan Klein & Shir Granit.
 
				
				----------  Ascii-Film App:  Client side  ----------
	
	
	compile with gcc client.c -o flix_control
	run with ./flix_control <server ip addr> <tcp control port>
  ==================================================================================*/
//--Includes:

//--Defines:
#define 

//--Global variables



//--Help functions
//screen preinter
//menu show and react
//request from client: change film

//request from client: goPRO
	//Open a new TCP soket
	//set its prop...
//request from client: speedupPRO
//request from client: change film PRO
//request from client: releasePRO



////~////~//// MAIN ////~////~////

//--declare main variables

//--Decode to variables input from command line

//-------------------------------------------------------------------
//--Open a new thread: TCP control
//this MUST be implemented in select! debug
//open a tcp socket
//set tcp socket properties

//send hello message -> wait for welcome mssg. ++Timeout
//--Establish connection with server ++Timeout


//connection astablished
//-------------------------------------------------------------------
//--Open a new thread: Movie presentor + Screen manue
//open a UDP socket
//set socket properties
//connect to the multicast group
//get message
// built screen 
// start streaming user
//input option - ENTER key
//Print manu 
//react to user choise -> send to help functions


// @ Ilan & Shir.