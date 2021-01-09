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
int quit = 0; //quit the program
// user initial input variables
in_addr_t ip_addr_server;
int port_tcp_control;
// buffer
uint8_t buff_tx[buffer_size]; //todo debug - uint and not char
uint8_t buff_rx[buffer_size]; //todo debug - uint and not char
// between threads
int mutex_io_film = 0;
int present_film = OFF;
int present_through_UDP = OFF;
int present_through_TCP = OFF;
int connect_to_a_new_film = OFF;
int connect_to_a_film_pro = OFF;
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
uint32_t multicastGroup_current; //announce
uint8_t filmNameSize; // announce
char filmName[buffer_size]; //announce
uint8_t permit; //PermitPro
uint16_t portNumber_Pro_TCP; //welcome
uint8_t ack_on_what; //welcome & announce
uint8_t replayStringSize; //welcome & announce
int err = 0; //error mssg

//--Help functions
//this function sends a requested massage to the socket mentioned
int send_msg(int socket, int mssg, uint8_t commandType, uint16_t station, uint8_t speed) {
    //function vars
    int write_to_socket;
    //switch cases
    switch (mssg) {
        case Hello:
            buff_tx[0] = commandType;
            buff_tx[1] = 0;
            buff_tx[2] = 0;
            break;
        case AskFilm:
            buff_tx[0] = commandType;
            buff_tx[1] = station & 0xff;
            buff_tx[2] = (station >> 8);
            //todo check station =
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
    //todo debug for(i=0;i<10;i++){printf("[] = %x ; ", buff_tx[i]);}//clear buffer
    //send the buffer to the socket
    write_to_socket = send(socket, buff_tx, buffer_size, 0);
    if (write_to_socket > 0) { return 1; }//success
    else { return 0; }//fail
}

//decode massage
void decode_ctrl_massage(uint8_t buff_rx_temp[buffer_size]) {
    //declare variables
    uint8_t numStations_1;
    uint8_t numStations_2;
    uint8_t multicastGroup_1;
    uint8_t multicastGroup_2;
    uint8_t multicastGroup_3;
    uint8_t multicastGroup_4;
    uint8_t portNumber_UDP_1;
    uint8_t portNumber_UDP_2;
    uint8_t portNumber_Pro_TCP_1;
    uint8_t portNumber_Pro_TCP_2;
    int i; //temp

    err = 0;
    //check the message type
    replyType = (int)(buff_rx[0]);

    //switch cases
    switch (replyType) {
        case Welcome:
            numStations_1 = buff_rx_temp[1];
            numStations_2 = buff_rx_temp[2];
            multicastGroup_1 = buff_rx_temp[3];
            multicastGroup_2 = buff_rx_temp[4];
            multicastGroup_3 = buff_rx_temp[5];
            multicastGroup_4 = buff_rx_temp[6];
            portNumber_UDP_1 = buff_rx_temp[7];
            portNumber_UDP_2 = buff_rx_temp[8];
            row = buff_rx_temp[9];
            col = buff_rx_temp[10];
            // fix fields
            numStations =
                    ((uint16_t) numStations_2 << 8) | (uint16_t) numStations_1;  // Combines 2 uint8_t into 1 uint16_t
            multicastGroup = ((uint32_t) multicastGroup_4 << 24) | ((uint32_t) multicastGroup_3 << 16) |
                             ((uint32_t) multicastGroup_2 << 8) | ((uint32_t) multicastGroup_1);
            multicastGroup_current = multicastGroup;
            portNumber_UDP = ((uint16_t) portNumber_UDP_2 << 8) | (uint16_t) portNumber_UDP_1;
            // check for errors
            if ((int)(numStations) < 0) { err = 1; }        // Check # of statios is logical value
            if (portNumber_UDP == 0) { err = 1; }    // Check port # is logical value
            if (row <= 0 || col <= 0) { err = 1; }
            if (err) {
                printf("An error occurred during  decoding 'Welcome message'\n");
                quit = 1;
            }
            printf("Received Welcome message:\n Stations number: %x, M.C. group addr.: %x, Port number: %x\n",
                   numStations, multicastGroup,portNumber_UDP);
            break;

        case Announce:
            row = buff_rx_temp[1];
            col = buff_rx_temp[2];
            filmNameSize = buff_rx_temp[3];
            for (i = 0; i < filmNameSize; i++) {
                filmName[i] = buff_rx_temp[4 + i];
            }
            // check for errors
            if (row <= 0 || col <= 0) { err = 1; }
            if (err) {//Disconnect
                printf("An error occurred during  decoding 'Announce message'\n");
                quit = 1;
            }
            break;

        case PermitPro:
            permit = buff_rx_temp[1];
            portNumber_Pro_TCP_1 = buff_rx_temp[2];
            portNumber_Pro_TCP_2 = buff_rx_temp[3];
            portNumber_Pro_TCP = ((uint16_t) numStations_2 << 8) | (uint16_t) numStations_1;
            // check for errors
            if (portNumber_Pro_TCP < 0) { err = 1; }
            if (permit != 0 || permit != 1) { err = 1; }
            if (err) {//Disconnect
                printf("An error occurred during  decoding 'PermitPro message'\n");
                quit = 1;
            }
            break;

        case Ack:
            ack_on_what = buff_rx_temp[1];
            // check for errors
            if (replyType != 3) { err = 1; }
            if (ack_on_what < 0 || ack_on_what > 4) { err = 1; }
            if (err) {//Disconnect
                printf("An error occurred during  decoding 'Ack message'\n");
                quit = 1;
            }
            break;

        case InvalidCommand:
            replayStringSize = buff_rx_temp[1];
            for (i = 0; i < replayStringSize; i++) {printf("%c", buff_rx_temp[1 + i]);}
            printf("\n");
            quit = 1;
            break;
    }
}

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
    s = sizeof(sock_struct);

    //infinite loop - if we are requested to ask film
    while (!quit) {
        if (present_film == ON) {
            printf("presenting to a new film\n"); //todo debug
            if(present_through_UDP == ON)
            {
                // start streaming user
                read_from_socket = recvfrom ( myUDPsocket , buff_rx_movie , buffer_size , 0 ,
                        (struct sockaddr *) &sock_struct , &s); //read UDP socket data into buffer
                printf("got mssg\n");
                // built screen
                char frame[row][col];
                int i, j;
                for( i = 0; i < row; i++){
                    for(j = 0; j < col; i++ ){
                        frame[i][col] = buff_rx_movie[i*row+j];
                    }
                }
                //print name of movie - row that stay todo check
                printf("Movie Name\n");
                for ( j = 0 ; j < col ; j++ ){printf("%c", frame[0][col]);}
                //(after the first frame)
                for ( i = 0 ; i < row ; i++ ){printf( "\033[1A\033[2K\r" );} //delete a row and move the 'cursor' up
                for ( i = 0 ; i < row ; i++ ) //print the frame
                {
                    for ( j = 0 ; j < col ; j++ ){printf("%c", frame[i][j]);}
                }
                printf("presenting to a new film\n"); //todo debug
            }
            if(present_through_TCP == ON)
            {
                //todo
                printf("presenting TCP\n"); //todo debug
            }

//            present_film = OFF;
        }//end present_film
        if (connect_to_a_new_film == ON) {
            printf("connecting to a new film\n"); //todo debug
            // ---UDP socket - receiver
            //open a UDP socket
            myUDPsocket = socket(AF_INET, SOCK_DGRAM, 0);
            //set socket properties
            sock_struct.sin_family = AF_INET;
            sock_struct.sin_port = htons(portNumber_UDP);
            sock_struct.sin_addr.s_addr = htonl(INADDR_ANY);
            //bind
            bind(myUDPsocket, (struct sockaddr *) &sock_struct, sizeof(sock_struct));
            //connect to a multicast group
            mreq.imr_multiaddr.s_addr = multicastGroup_current; //todo debug
            mreq.imr_interface.s_addr = htonl(INADDR_ANY);
            //connect to m.c. group
            printf("UDP listener is waiting...\n");
            setsockopt( myUDPsocket , IPPROTO_IP , IP_ADD_MEMBERSHIP , &mreq , sizeof(mreq));///!!!
            //change mode
            if (!mutex_io_film) {
                mutex_io_film = 1;
                connect_to_a_new_film = OFF;
                present_through_TCP = OFF;
                present_through_UDP = ON;
                present_film = ON;
                mutex_io_film = 0;
            }
        }// end connect_to_a_new_film
        if (connect_to_a_film_pro == ON) {
            //todo
            //socket TCP
            if (!mutex_io_film) {
                mutex_io_film = 1;
                connect_to_a_film_pro = OFF;
                present_through_UDP = OFF;
                present_through_TCP = ON;
                present_film = ON;
                mutex_io_film = 0;
            }
            // present from TCP
            printf("im pro\n");
        }//end connect_to_a_film_pro
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
    // menu variables
    char enter;
    int clientMenu = 0;
    int menuPro = 0;
    int stationPro = 0;
    float speedUp = 0;
    int permitPro = 0;
    int station = 0;

    /*SOCKET PROCESS*/
    //set sockaddr struct:
    sock_struct.sin_family = AF_INET;
    sock_struct.sin_port = htons(port_tcp_control);
    sock_struct.sin_addr.s_addr = ip_addr_server;//todo check
    // open a TCP socket - to the server
    int myTCPsocket = socket(AF_INET, SOCK_STREAM, 0);
    //connect to sender
    int c = connect(myTCPsocket, (struct sockaddr *) &sock_struct, sizeof(sock_struct));
    if(c == -1){printf("Sorry, could not connect to the server. Please make sure the server side is on.\n");quit=1;return 0;}

    // -- Connect to the server
    //Initialize the file descriptor set. for timeout
    FD_ZERO (&sock_set);
    FD_SET (myTCPsocket, &sock_set);

    // send a HELLO message
    t = send_msg(myTCPsocket, Hello, 0, 0, 0);
    // start timeout
    select_output = select(myTCPsocket + 1, &sock_set, NULL, NULL, &timeout); // todo check if really work
    if (select_output == 0) {printf("Timeout has expired - 'Welcome' .\n");quit=1;return 0;}// err massage
    // network layer: connection established

    // if receive WELCOME - read from socket
    read_from_socket = recv ( myTCPsocket , buff_rx , buffer_size , 0 ); ///!!!
    if (read_from_socket < 0) {
        printf("Err in 'recv' function (Welcome).\n");
        quit=1;
        return 0;
    }// err massage
    decode_ctrl_massage(buff_rx);

    // get first film - mutex
    if (!mutex_io_film) {
        mutex_io_film = 1;
        connect_to_a_new_film = ON; // after present the movie
        mutex_io_film = 0;
    }

    while (!quit) {//-- Menu --
        scanf("%c", &enter); //get enter
        present_film = OFF; // if user press enter -> movie stop
        printf("Please enter your choice:\n1. Change Movie\n2. Ask For Premium\n3. Return to watch\n4. Quit Program\n");
        scanf("%d", &clientMenu);
        if (clientMenu == 1)//Change Movie
        {
            printf("Please enter a station:  \n");
            scanf("%d", &station);
            //Send AskFilm
            t = send_msg(myTCPsocket, AskFilm, 0x01, (uint8_t)(station), 0);
            // start timeout
            select_output = select(myTCPsocket + 1, &sock_set, NULL, NULL, &timeout); // todo check if really work
            if (select_output == 0) {printf("Timeout has expired - 'Announce' .\n"); quit=1; return 0;}// err massage
            // if receive Announce -> read from socket
            read_from_socket = recv ( myTCPsocket , buff_rx , buffer_size , 0 ); ///check todo
            if (read_from_socket < 0) {
                printf("Err in 'recv' function (Announce).\n");
                return 0;
            }// err massage
            row = col = 0;//reset film variables
            decode_ctrl_massage(buff_rx);//get response
            // set film to present
            multicastGroup_current = multicastGroup_current + (uint32_t)(station);//todo fixme check the add
            connect_to_a_new_film = ON;
            printf("changed film");//todo debug
        }
        if (clientMenu == 2)//Ask For Premium
        {
            //Send GoPro
            t = send_msg(myTCPsocket, GoPro, 0x02, 0, 0);
            // start timeout
            select_output = select(myTCPsocket + 1, &sock_set, NULL, NULL, &timeout); // todo check if really work
            if (select_output == 0) {printf("Timeout has expired - 'GoPro' .\n"); quit=1; return 0;}// err massage
            // if receive Announce -> read from socket
            read_from_socket = recv ( myTCPsocket , buff_rx , buffer_size , 0 ); ///check todo
            if (read_from_socket < 0) {printf("Err in 'recv' function (GoPro).\n"); return 0;}// err massage
            //set premit pro
            decode_ctrl_massage(buff_rx);//get response permitPro
            if(permitPro) {
                connect_to_a_film_pro = ON;
                while (permitPro) {//-- PRO Menu --
                    scanf("%c", &enter); //get enter
                    present_film = OFF; // if user press enter -> movie stop
                    printf("Please enter a station:\n");
                    printf("1. Change Movie\n");
                    printf("2. Go Faster\n");
                    printf("3. Leave Pro\n");
                    printf("4. Return to watch\n");
                    printf("5. Quit Program\n");
                    scanf("%d", &menuPro);
                    //user choice
                    if (menuPro == 1)//Pro: Change Movie
                    {
                        printf("Please enter a station:  \n");
                        scanf("%d", &stationPro);
                        //Send AskFilm
                        t = send_msg(myTCPsocket, AskFilm, 0x01, (uint8_t)(station), 0);
                        // start timeout
                        select_output = select(myTCPsocket + 1, &sock_set, NULL, NULL, &timeout); // todo check if really work
                        if (select_output == 0) {printf("Timeout has expired - 'Announce' .\n"); quit=1; return 0;}// err massage
                        // if receive Announce -> read from socket
                        read_from_socket = recv ( myTCPsocket , buff_rx , buffer_size , 0 ); ///check todo
                        if (read_from_socket < 0) {printf("Err in 'recv' function (Announce).\n"); return 0;}// err massage
                        row = col = 0;//reset film variables
                        decode_ctrl_massage(buff_rx);//get response
                        // set film to present
                        connect_to_a_film_pro = ON;
                        printf("changed to pro");//todo debug
                    }
                    if (menuPro == 2)//Pro: Go Faster
                    {
                        printf("speed up  \n");
                        scanf("%f", &speedUp);
                        speedUp = speedUp * 0.01 + 1;  // 100% + 0%-100% for speed-up
                        //Send AskFilm
                        t = send_msg(myTCPsocket, AskFilm, 0x01, 0, (uint8_t)(speedUp));
                        // start timeout
                        select_output = select(myTCPsocket + 1, &sock_set, NULL, NULL, &timeout); // todo check if really work
                        if (select_output == 0) {printf("Timeout has expired - 'speedUp' .\n"); quit=1; return 0;}// err massage
                        // if receive Announce -> read from socket
                        read_from_socket = recv ( myTCPsocket , buff_rx , buffer_size , 0 ); ///check todo
                        if (read_from_socket < 0) {printf("Err in 'recv' function (speedUp).\n"); return 0;}// err massage
                        decode_ctrl_massage(buff_rx);//get response
                        // set film to present
                        present_film = ON;
                    }
                    if (menuPro == 3)//Pro: Leave Pro
                    {
                        printf("Leave Pro:  \n");
                        //Dissconnect TCP Socket todo after TCP socket
                        permitPro = 0;
                    }
                    if (menuPro == 4)//Pro: Return to watch
                    {
                        printf("Return to watch:  \n");
                        // set film to present
                        present_film = ON;
                    }
                    if (menuPro == 5)//Pro:Quit Program
                    {
                        printf("Quit Program:  \n");
                        //Dissconect
                        quit = 1;
                    }
                }//end while pro
            }//end if permit pro
            if(!permitPro){printf("GoPro was not approved.\n");}//todo check what happends next
        }
        if (clientMenu == 3)//Return to watch
        {
            printf("Return to watch:  \n");
            present_film = ON;
        }
        if (clientMenu == 4)//Quit Program
        {
            printf("Quit Program:  \n");
            quit = 1;
            //Dissconect from sockets
        }

        //todo print the message received

        // todo if the server closed the tcp socket -> exit
        // if quit -> close connection & print mssg & release all & close sockets

    }//end while 1
}//end thread server control

////~////~//// MAIN ////~////~////
int main(int argc, char **argv) {
    //--declare main variables
    pthread_t thread_id_movie_streamer, thread_id_server_control; //create thread variable
    int i;//temp holders
    //--Decode to variables input from command line
    ip_addr_server = inet_addr(argv[1]); // the server IP number
    port_tcp_control = atoi(argv[2]); // port number of the TCP control socket
    //--set initial settings
    for (i = 0; i < buffer_size; i++) { buff_tx[i] = 0; }//clear buffer

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