/*====================================================================================
 Final project in Lab session of CCS2 course.
 Written by Ilan Klein & Shir Granit.


				----------  Ascii-Film App:  Client side  ----------

	compile with gcc client.c -o flix_control
	run with ./flix_control <server ip addr> <tcp control port>

    e.g. ./flix_control 192.10.1.1 5555

  ==================================================================================*/
//--Includes:
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> //for sleep()
#include <pthread.h> //for threads
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <fcntl.h> //read files
#include <ifaddrs.h>
#include <errno.h> //err
#include <sys/time.h> //for timer

//--Defines:
#define buffer_size 1024
#define timeout_time 0.3
// massages- words for human
#define ON 1
#define OFF 0;
// send types
#define Hello 10
#define AskFilm 20
#define GoPro 30
#define SpeedUp 40
#define Release 50
// receive types
#define Welcome 60
#define Announce 70
#define PermitPro 80
#define Ack 90
#define InvalidCommand 100


//--Global variables
// user input throughout the program
char req_fir_menu; // if the user press "Enter" while movie streaming
int user_menu_choice;
// user initial input variables
in_addr_t ip_addr_server;
int port_tcp_control;
// buffer
uint8_t buff_tx[buffer_size]; //todo debug - uint and not char
uint8_t buff_rx[buffer_size]; //todo debug - uint and not char
// between threads
int mutex_io_film = 0;
int present_film = OFF;
int connect_to_a_new_film = OFF;
char input_char;
//massage types - tx
uint8_t commandType;
//massage types - rx
uint8_t replyType;
uint16_t numStations; //welcome
uint32_t multicastGroup; //welcome
uint16_t portNumber_UDP; //welcome
uint8_t row; //welcome & announce
uint8_t col; //welcome & announce
uint8_t filmNameSize; // announce
char filmName[buffer_size]; //announce
uint8_t permit; //PermitPro
uint16_t portNumber_Pro_TCP; //welcome
uint8_t ack_on_what; //welcome & announce
uint8_t replayStringSize; //welcome & announce
char replyString[buffer_size]; //announce
int err = 0; //error mssg

//--Help functions
//this function sends a requested massage to the socket mentioned
int send_msg(int socket,int mssg, uint8_t commandType, uint16_t station, uint8_t speed)
{
    //function vars
    int write_to_socket;
    //switch cases
    switch(mssg)
    {
        case Hello:
            buff_tx[0] = commandType;
            buff_tx[1] = 0;
            buff_tx[2] = 0;
            break;
        case AskFilm:
            buff_tx[0] = commandType;
            buff_tx[1]=station & 0xff;
            buff_tx[2]=(station >> 8);
            break;
        case GoPro:
            buff_tx[0] = commandType;
            buff_tx[1] = 0;
            buff_tx[2] = 0;
            break;
        case SpeedUp:
            buff_tx[0] = commandType;
            buff_tx[1] = speed;
            buff_tx[2] = 0;
            break;
        case Release:
            buff_tx[0] = commandType;
            buff_tx[1] = 0;
            buff_tx[2] = 0;
            break;
    }
    //todo debug for(int i=0;i<10;i++){printf("[] = %x ; ", buff_tx[i]);}//clear buffer
    //send the buffer to the socket
    write_to_socket = send( socket , buff_tx , buffer_size , 0 );
    if(write_to_socket>0){return 1;}//success
    else{return 0;}//fail
}
//decode massage
void decode_ctrl_massage(int mssg)
{
    //todo add mutex for buff_rx?
    //todo check
    //switch cases

    err = 0;

    switch(mssg)
    {
        case Welcome:
            replyType = buff_rx[0];
            uint8_t numStations_1 = buff_rx[1];
            uint8_t numStations_2 = buff_rx[2];
            uint8_t multicastGroup_1 = buff_rx[3];
            uint8_t multicastGroup_2 = buff_rx[4];
            uint8_t multicastGroup_3 = buff_rx[5];
            uint8_t multicastGroup_4 = buff_rx[6];
            uint8_t portNumber_UDP_1 = buff_rx[7];
            uint8_t portNumber_UDP_2 = buff_rx[8];
            row = buff_rx[9];
            col = buff_rx[10];

            // fix fields
            numStations = ((uint16_t)numStations_2 << 8) | (uint16_t)numStations_1;  // Combines 2 uint8_t into 1 uint16_t
            multicastGroup = ((uint32_t)multicastGroup_4 << 24) | ((uint32_t)multicastGroup_3 << 16) | ((uint32_t)multicastGroup_2 << 8) | ((uint32_t)multicastGroup_1);
            portNumber_UDP = ((uint16_t)portNumber_UDP_2 << 8) | (uint16_t)portNumber_UDP_1;

            // check for errors
            if(!replyType){err = 1;}				// Check Type of message
            if(numStations < 0 ){err = 1;} 		// Check # of statios is logical value
            if(multicastGroup < 0 ){err = 1;} 	// Check Multicast group is logical value
            if(portNumber_UDP < 0 ){err = 1;} 	// Check port # is logical value
            if(row <= 0 || col <= 0){err=1;}
            if(err){//Disconnect todo
            }
            break;

        case Announce:
            replyType = buff_rx[0];
            row = buff_rx[1];
            col = buff_rx[2];
            filmNameSize = buff_rx[3];
            for(int i=0; i<filmNameSize; i++)
            {
                filmName[i] = buff_rx[4+i];
            }

            // check for errors
            if(replyType != 1){err=1;}
            if(row <= 0 || col <= 0){err=1;}
            if(err){//Disconnect todo
            }
            break;

        case PermitPro:
            replyType = buff_rx[0] ;
            permit = buff_rx[1];
            uint8_t portNumber_Pro_TCP_1 = buff_rx[2];
            uint8_t portNumber_Pro_TCP_2 = buff_rx[3];
            portNumber_Pro_TCP = ((uint16_t)numStations_2 << 8) | (uint16_t)numStations_1;

            // check for errors
            if(replyType != 2){err=1;}
            if(portNumber_Pro_TCP < 0 ){err = 1;}
            if(permit != 0 || permit != 1 ){err = 1;}
            if(err){//Disconnect todo
            }
            break;

        case Ack:
            replyType =buff_rx[0];
            ack_on_what = buff_rx[1];

            // check for errors

            if(replyType != 3){err=1;}
            if( ack_on_what < 0 || ack_on_what > 4 ){err = 1;}
            if(err){//Disconnect todo
            }
            break;

        case InvalidCommand:
            replyType = buff_rx[0] ;
            replayStringSize = buff_rx[1];
            for(int i=0; i<replayStringSize; i++)
            {
                replyString[i] = buff_rx[1+i];
            }
            break;
    }
}


//screen preinter
//menu show and react

//request from client: change film

//request from client: goPRO
//Open a new TCP soket
//set its prop...
//request from client: speedupPRO
//request from client: change film PRO
//request from client: releasePRO

//--MAIN Help functions
// Movie streaming thread function
void *movie_streamer_thread_function(void *v) {
    printf("hi ilannn movie_streamer_thread_function\n");
    //function variables - local
    int myUDPsocket;
    int expected_byte_num;
    int actual_byte_num;
    char buff_rx_movie[buffer_size];//buffer for data
    ssize_t read_from_socket;//read from socket
    struct ip_mreq mreq;//create multicast request
    struct sockaddr_in sock_struct;//create sockaddr struct:
    int s;
    s= sizeof(sock_struct);

    //infinite loop - if we are requested to ask film
    while(ON) {
        if(present_film==ON) {
            printf("presenting to a new film\n"); //todo debug
            //read socket data into buffer
            //read_from_socket = recvfrom ( myUDPsocket , buff_rx_movie , buffer_size , 0 , (struct sockaddr *) &sock_struct , &s);
            //todo bug - print data to screen
            present_film = OFF;
            //get message
            // built screen
            // start streaming user
        }
        if(connect_to_a_new_film==ON){
            printf("connecting to a new film\n"); //todo debug
            // ---UDP socket - receiver
            //open a UDP socket
            myUDPsocket = socket(AF_INET , SOCK_DGRAM , 0);
            //set socket properties
            sock_struct.sin_family = AF_INET ;
            sock_struct.sin_port = htons(portNumber_UDP) ;
            sock_struct.sin_addr.s_addr = htonl(INADDR_ANY);
            //bind
            bind( myUDPsocket , (struct sockaddr *) &sock_struct , sizeof(sock_struct) );
            //connect to a multicast group
            mreq.imr_multiaddr.s_addr = multicastGroup; //todo debug
            mreq.imr_interface.s_addr = htonl(INADDR_ANY);
            //connect to m.c. group
            printf("UDP listener is waiting...\n");
            ///!!!setsockopt( myUDPsocket , IPPROTO_IP , IP_ADD_MEMBERSHIP , &mreq , sizeof(mreq));///!!!
            //change mode
            if(!mutex_io_film) {
                mutex_io_film = 1;
                connect_to_a_new_film = OFF;
                present_film = ON;
                mutex_io_film = 0;
            }
        }
    }//end

    //input option - ENTER key
}

// TCP control thread function
void *server_control_thread_function(void *v) {
    printf("hi ilannn server_control_thread_function\n");//todo del

    // declare inside variables
    struct sockaddr_in sock_struct;
    int i, t; //temp variables
    int read_from_socket; //mark how much we read from the socket
    // Timeout and select definitions
    struct timeval timeout;
    timeout.tv_sec = timeout_time;// fixme - its now 0
    timeout.tv_usec = 0;
    fd_set sock_set;
    int select_output;

    /*SOCKET PROCESS*/
    //set sockaddr struct:
    sock_struct.sin_family = AF_INET;
    sock_struct.sin_port = htons(port_tcp_control);
    sock_struct.sin_addr.s_addr = ip_addr_server;//todo check
    // open a TCP socket - to the server
    int myTCPsocket = socket(AF_INET, SOCK_STREAM, 0);
    //connect to sender
    int c = connect(myTCPsocket, (struct sockaddr *) &sock_struct, sizeof(sock_struct));
    //todo if(c == -1){printf("Sorry, could not connect to the server. Please make sure the server side is on.\n");return 0;}

    // -- Connect to the server
    //Initialize the file descriptor set. for timeout
    FD_ZERO (&sock_set);
    FD_SET (myTCPsocket, &sock_set);

    // send a HELLO message
    t = send_msg(myTCPsocket, Hello, 0, NULL, NULL);
    // start timeout
    select_output = select(myTCPsocket + 1, &sock_set, NULL, NULL, &timeout); // todo check if really work
    //todo if (select_output == 0) {printf("Timeout has expired - 'Welcome' .\n");return 0;}// err massage
    // network layer: connection established

    // if receive WELCOME -> continue to ask film on station 1
    //read from socket
    //!!! read_from_socket = recv ( myTCPsocket , buff_rx , buffer_size , 0 ); ///!!!
    if(buff_rx[0]!=0){printf("Did get a massage but NOT 'Welcome'.\n");return 0;}// err massage
    if(read_from_socket<0){printf("Err in 'recv' function (Welcome).\n");return 0;}// err massage
    decode_ctrl_massage(Welcome);//todo

    //todo print the message received

    // get first film - mutex
    if(!mutex_io_film){
        mutex_io_film = 1;
        connect_to_a_new_film = ON; // after present the movie
        mutex_io_film = 0;
    }

    while(1) {
        //-- Menu -
        scanf("%c", &input_char);

        //react to user choice -> send to help functions

        //option to press "Enter" and open menu
        //give the user options
        //get response -> act accordingly --- switch
        //get response for the propriate request
        //go back to //option to press "Enter" and open menu

        //todo print the message received

        // todo if the server closed the tcp socket -> exit
        // if quit -> close connection & print mssg & release all & close sockets

        printf("MENU\n");
        printf("---%c---\n", input_char);
        present_film=ON;
    }

}

////~////~//// MAIN ////~////~////
int main(int argc, char **argv) {
    //--declare main variables
    pthread_t thread_id_movie_streamer, thread_id_server_control; //create thread variable
    int i;//temp holders
    //--Decode to variables input from command line
    ip_addr_server = inet_addr(argv[1]); // the server IP number
    port_tcp_control = atoi(argv[2]); // port number of the TCP control socket
    //--set initial settings
    for(i=0;i<buffer_size;i++){buff_tx[i]=0;}//clear buffer

    //-------------------------------------------------------------------
    //--Open a new thread: Movie presentor + Screen menu
    pthread_create(&thread_id_server_control, NULL, server_control_thread_function, NULL);

    //-------------------------------------------------------------------
    //--Open a new thread: TCP control
    pthread_create(&thread_id_movie_streamer, NULL, movie_streamer_thread_function, NULL);


    //todo pthread join !
    //-------------------------------------------------------------------
    pthread_join(thread_id_movie_streamer, NULL);//block until thread is finished - stop bugs
    pthread_join(thread_id_server_control, NULL);//block until thread is finished - stop bugs

    return 0;
}//end main
// @ Ilan & Shir.