/*====================================================================================
  ====================================================================================
 Final project in Lab session of CCS2 course.
 Written by Ilan Klein & Shir Granit.


				----------  Ascii-Film App:  Client side  ----------

	compile with gcc client.c -o flix_control -lpthread

	run with ./flix_control <server ip addr> <tcp control port>

    e.g. ./flix_control 192.10.1.1 5555

 ====================================================================================
 ====================================================================================*/
//--Includes:
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> //for sleep()
#include <pthread.h> //for threads
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h> //err

//--Defines:
#define buffer_size 1024
#define timeout_time 300000
// massages- words for human
#define ON 1
#define OFF 0;
// send types
#define Hello 0
#define AskFilm 1
#define GoPro 2
#define SpeedUp 3
#define Release 4
// receive types
#define Welcome 0
#define Announce 1
#define PermitPro 2
#define Ack 3
#define InvalidCommand 4

//--Global variables
extern int errno;
int quit = 0;                               //quit the program
// user initial input variables
in_addr_t ip_addr_server;
int port_tcp_control;
uint8_t buff_rx[buffer_size];
// between threads
int mutex_io_film = 0;
int present_film = OFF;
int present_through_UDP = OFF;
int present_through_TCP = OFF;
//massage types - rx
uint8_t replyType;
uint16_t numStations;                       //welcome
uint32_t multicastGroup;                    //welcome
uint16_t portNumber_UDP;                    //welcome
uint8_t row;                                //welcome & announce
uint8_t col;                                //welcome & announce
uint8_t filmNameSize;                       // announce
char filmName[buffer_size];                 //announce
uint8_t permit = OFF;                       //PermitPro
uint16_t portNumber_Pro_TCP;                //welcome
uint8_t ack_on_what;                        //welcome & announce
uint8_t replayStringSize;                   //welcome & announce
int flag_first_frame_printout = 0;
int flag_first_frame_printout_pro = 0;
int current_station;
int myUDPsocket;
struct ip_mreq mreq;                        //create multicast request
struct sockaddr_in sock_struct_UDP;         //create sockaddr struct:
int s;                                      //size of struct
struct sockaddr_in sock_struct_TCP;         //create sockaddr struct:
int proTCPsocket;


//~~ Help functions
void *error_handler(int type) {
    switch (type) {
        case 0:
            printf("Sorry, could not connect to the server. Please make sure the server side is on.\n");
            break;
        case 1:
            printf("Sorry, could not sent the message you requested.\n");
            break;
        case 2:
            printf("Sorry, could not decode the message you received correctly.\n");
            break;
        case 3:
            printf("Response Invalid Command received. shutting down.\n");
            break;
        case 4:
            printf("Sorry, could not receive message.\n");
            break;

    }
    quit = 1;
    return NULL;
}//error handler

int time_out(int sock) {
    // Timeout and select definitions
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = timeout_time;
    //Initialize the file descriptor set. for timeout
    fd_set sock_set;
    FD_ZERO (&sock_set);
    FD_SET (sock, &sock_set);
    int select_output;
    // start timeout
    select_output = select(sock + 1, &sock_set, NULL, NULL, &timeout);
    if (select_output == 0) {
        printf("Timeout has expired.\n");
        quit = 1;
        return 1;
    }// err massage
    else {
        return 0;
    }
}//time_out

int send_msg(int socket, int mssg, uint8_t command, uint16_t station, uint8_t speed) {
    uint8_t buff_tx[buffer_size];
    int errnum;
    //function vars
    int write_to_socket;
    //switch cases
    buff_tx[0] = command;
    switch (mssg) {
        case Hello:
            buff_tx[1] = 0;
            buff_tx[2] = 0;
            printf("Sending the server 'Hello' message...");
            break;
        case AskFilm:
            buff_tx[1] = (station >> 8);
            buff_tx[2] = station & 0xff;
            printf("Sending the server 'AskFilm' message (%d)...", station);
            break;
        case GoPro:
            buff_tx[1] = 0;
            buff_tx[2] = 0;
            printf("Sending the server 'GoPro' message...");
            break;
        case SpeedUp:
            buff_tx[1] = speed;
            buff_tx[2] = 0;
            printf("Sending the server 'SpeedUp' message (%d)...", speed);
            break;
        case Release:
            buff_tx[1] = 0;
            buff_tx[2] = 0;
            printf("Sending the server 'Release' message...");
            break;
        default:
            printf("Error in sending function.\n");
            break;
    }
    //send the buffer to the socket
    write_to_socket = send(socket, buff_tx, buffer_size, 0);
    if (write_to_socket == -1) {
        errnum = errno;
        fprintf(stderr, "Value of errno: %d\n", errno);
        perror("Error printed by perror\n");
        fprintf(stderr, "Error opening file: %s\n", strerror(errnum));
    } else { printf("success.\n"); }
    if (write_to_socket > 0) { return 1; }//success
    else { return 0; }//fail
}//send_msg

void decode_ctrl_massage(uint8_t buff_rx_temp[buffer_size]) {
    //declare variables
    int i; //temp
    //check the message type
    replyType = (int) (buff_rx[0]);
    //switch cases
    switch (replyType) {
        case Welcome:
            numStations = ntohs(*(uint16_t *) &buff_rx_temp[1]);
            multicastGroup = ntohl(*(uint32_t *) &buff_rx_temp[3]);
            portNumber_UDP = ntohs(*(uint16_t *) &buff_rx_temp[7]);
            row = (uint8_t) buff_rx_temp[9];
            col = (uint8_t) buff_rx_temp[10];
            printf("Received Welcome message:\nNumber of stations: %x (start from 0).\nM.C. group addr.: %x.\nPort number: %d.\n",
                   numStations, multicastGroup, portNumber_UDP);
            printf("Frame: Row length is %d; Column length is %d.\n", row, col);
            break;

        case Announce:
            row = buff_rx_temp[1];
            col = buff_rx_temp[2];
            filmNameSize = buff_rx_temp[3];
            for (i = 0; i < filmNameSize; i++) {
                filmName[i] = buff_rx_temp[4 + i];
            }
            if (row <= 0 || col <= 0) { error_handler(2); }// check for errors
            printf("Received Announce message:\nRow dimensions: %d.\nColumn dimensions: %d.\n", row, col);
            printf("Movie Name: ");
            for (i = 0; i < filmNameSize; i++) { printf("%c", filmName[i]); }
            printf("\n");
            break;

        case PermitPro:
            permit = buff_rx_temp[1];
            portNumber_Pro_TCP = ntohs(*(uint16_t *) &buff_rx_temp[2]);
            // check for errors
            printf("Received PermitPro message: %d (port %d).\n", permit, portNumber_Pro_TCP);
            break;

        case Ack:
            ack_on_what = buff_rx_temp[1];
            // check for errors
            if (ack_on_what < 0 || ack_on_what > 4) { error_handler(2); }
            printf("Received ACK message: %d.\n", ack_on_what);
            break;

        case InvalidCommand:
            replayStringSize = buff_rx_temp[1];
            for (i = 0; i < replayStringSize; i++) { printf("%c", buff_rx_temp[1 + i]); }
            printf("\n");
            error_handler(3);
            break;

        default:
            error_handler(2);
    }
}//decode_ctrl_massage

void *register_to_a_new_movie_socket(int station, int type_socket, int flag_not_first_time) {
    if (type_socket == 1) {//UDP
        current_station = station;
        //present film name
        s = sizeof(sock_struct_UDP);

        if (flag_not_first_time) {
            //free resorces and quit M.C. old group
            setsockopt(myUDPsocket, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq));
            close(myUDPsocket);
        }
        // ---UDP socket - receiver
        //open a UDP socket
        myUDPsocket = socket(AF_INET, SOCK_DGRAM, 0);
        //set socket properties
        sock_struct_UDP.sin_family = AF_INET;
        sock_struct_UDP.sin_port = htons(portNumber_UDP);
        sock_struct_UDP.sin_addr.s_addr = htonl(INADDR_ANY);
        //bind
        bind(myUDPsocket, (struct sockaddr *) &sock_struct_UDP, sizeof(sock_struct_UDP));
        //connect to a multicast group
        mreq.imr_multiaddr.s_addr = multicastGroup + (uint32_t) (station);    //takes uint32_bit
        mreq.imr_interface.s_addr = htonl(INADDR_ANY);
        //connect to m.c. group
        //printf("UDP listener is waiting...\n");
        setsockopt(myUDPsocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));///!!!
        //change mode
        if (mutex_io_film) {
            while (mutex_io_film) { ; };
        }
        mutex_io_film = 1;
        present_through_TCP = OFF;
        present_through_UDP = ON;
        flag_first_frame_printout = 0;
        mutex_io_film = 0;
    }//UDP
    if (type_socket == 2) {//TCP
        present_through_UDP = OFF;
        present_through_TCP = ON;
        flag_first_frame_printout_pro = 0;
    }//TCP
}//register_to_a_new_movie_socket

void change_film(int station, int socket) {
    int read_from_socket;
    //Send AskFilm
    send_msg(socket, AskFilm, 0x01, (uint16_t) (station), 0);
    if (!time_out(socket)) { read_from_socket = recv(socket, buff_rx, buffer_size, 0); }
    if (read_from_socket <= 0) { printf("Err in 'recv' function (Announce).\n"); }// err massage
    decode_ctrl_massage(buff_rx);//get response
}//change_film

void faster(int sock) {
//    present_film = OFF;
    float speedUp;
    int read_from_socket;
    char enter;
    printf("Please enter a number in range 0-100:  \n");
    scanf("%f", &speedUp); // 100% + 0%-100% for speed-up
    scanf("%c", &enter); // 100% + 0%-100% for speed-up
    //Send AskFilm
    send_msg(sock, SpeedUp, 0x03, 0, (uint8_t) (speedUp));
    if (!time_out(sock)) { read_from_socket = recv(sock, buff_rx, buffer_size, 0); }
    if (read_from_socket <= 0) { error_handler(3); }// err massage
    decode_ctrl_massage(buff_rx);//get response
    printf("Speed up ACK received.\n");
}//faster

void leave_pro(int sock) {
//    present_film = OFF;
    int read_from_socket;
    //Send AskFilm
    send_msg(sock, Release, 0x04, 0, 0);
    if (!time_out(sock)) { read_from_socket = recv(sock, buff_rx, buffer_size, 0); }
    if (read_from_socket <= 0) { error_handler(3); }// err massage
    printf("Left Pro service successfully.\n");
    //UDP return
}//leave_pro

void *go_pro(int socket_ctrl) {
    int read_from_socket, permitPro, menuPro;
    char enter, line[100];
    send_msg(socket_ctrl, GoPro, 0x02, 0, 0);//Send GoPro
    if (!time_out(socket_ctrl)) { read_from_socket = recv(socket_ctrl, buff_rx, buffer_size, 0); }
    if (read_from_socket <= 0) { printf("Err in 'recv' function (GoPro).\n"); }// err massage
    decode_ctrl_massage(buff_rx);//get response permitPro
    permitPro = (int) (permit);//set permit pro
    if (!permitPro) { printf("GoPro was not approved.\n"); }
    if (permitPro) {
        //free resorces and quit M.C. old group
        setsockopt(myUDPsocket, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq));
        close(myUDPsocket);
        /*TCP SOCKET PROCESS*/
        //set sockaddr struct:
        sock_struct_TCP.sin_family = AF_INET;
        sock_struct_TCP.sin_port = htons(portNumber_Pro_TCP);
        sock_struct_TCP.sin_addr.s_addr = ip_addr_server;
        // open a TCP socket - to the server
        proTCPsocket = socket(AF_INET, SOCK_STREAM, 0);
        //connect to sender
        int c = connect(proTCPsocket, (struct sockaddr *) &sock_struct_TCP, sizeof(sock_struct_TCP));
        if (c == -1) {
            error_handler(0);
            return NULL;
        }
        printf("Connected to the server successfully!\n");

        register_to_a_new_movie_socket(0, 2, 0);//TCP first time
        flag_first_frame_printout_pro = 0;
        present_film = ON;

        while (permitPro) {//-- PRO Menu --
            fgets(line, sizeof(line), stdin);
            present_film = OFF; // if user press enter -> movie stop
            fflush(stdin);
            printf("Pro Menu:\n1. Change Movie\n2. Go Faster\n3. Leave Pro\n4. Return to watch\n5. Quit Program\n");
            fgets(line, sizeof(line), stdin);
            sscanf(line, "%d", &menuPro);
            fflush(stdin);
            if (menuPro == 1) {
                int station;
                printf("Please enter a station:  \n");
                fgets(line, sizeof(line), stdin);
                sscanf(line, "%d", &station);
                fflush(stdin);
                printf("this is station = %d\n", station);
                change_film(station, socket_ctrl);
                register_to_a_new_movie_socket(station, 2, 1);//station, TCP, not first time
                flag_first_frame_printout_pro = 0;
                present_film = ON;
            }//Pro: Change Movie
            if (menuPro == 2) { faster(socket_ctrl); } //Pro: Go Faster
            if (menuPro == 3) {
                leave_pro(socket_ctrl);
                permitPro = 0;
            }//Pro: Leave Pro
            if (menuPro == 4) {
                printf("Return to watch:  \n");
                flag_first_frame_printout_pro = 0;
                present_film = ON;
            }//Pro: Return to watch
            if (menuPro == 5) {
                printf("Quit Program. Goodbye!\n");
                //Dissconect
                quit = 1;
                permitPro = 0;
                close(proTCPsocket);
            }//Pro:Quit Program
            if ((menuPro != 1) && (menuPro != 2) && (menuPro != 3) && (menuPro != 4) && (menuPro != 5)) {
                printf("Wrong input.\n");
            }
        }//end while pro
        register_to_a_new_movie_socket(0, 1, 0);//TCP first time
        flag_first_frame_printout = 0;
        present_film = ON;
    }//end if permit pro
    return NULL;
}//go_pro

//~~ Thread functions:
void *movie_streamer_thread_function(void *v) {
    int expected_byte_num;
    int actual_byte_num;
//    char buff_rx_movie[buffer_size];//buffer for data
    ssize_t read_from_socket;//read from socket
    int flag_mc_first_reg = 0;
    int i, j; //temp holder
    int ptr_buf;
    char cr;
    int flag_EOL;
    int row_del;

    //infinite loop - if we are requested to ask film
    while (!quit) {
        if (present_film == ON) {
            if (present_through_UDP == ON) {
                // start streaming user
                char buff_rx_movie[buffer_size];//buffer for data
                read_from_socket = recvfrom(myUDPsocket, buff_rx_movie, buffer_size, 0,
                                            (struct sockaddr *) &sock_struct_UDP,
                                            &s); //read UDP socket data into buffer
                //(after the first frame) delete old frame
                if (present_film == ON) { // make sure that screen is not taken
                    if (flag_first_frame_printout) {
                        for (i = 0; i < row_del; i++) { printf("\033[1A\033[2K\r"); }
                    }//delete a rows and move the 'cursor' up
                    //print frame
                    // work with target for (i = 0; i < row; i++) { for (j = 0; j < col; j++) { printf("%c", buff_rx_movie[i * col + j]); }}
                    //build buf
                    row_del = 0;
                    ptr_buf = 0;
                    for (i = 0; i < row - 1; i++) {
                        flag_EOL = 0;
                        for (j = 0; j < col; j++) {
                            if (!flag_EOL) {
                                cr = buff_rx_movie[ptr_buf];
                                if (cr == '\n') {
                                    flag_EOL = 1;
                                    printf("\n");
                                    row_del = row_del + 1;
                                }//end of line
                                if (!flag_EOL) { printf("%c", buff_rx_movie[ptr_buf]); }//line itself
                                ptr_buf = ptr_buf + 1;
                            }
                        }
                    }

                    flag_first_frame_printout = 1;
                }
            }//end present_through_UDP
            if (present_through_TCP == ON) {
                // start streaming user
                char buff_rx_movie[row*col];//buffer for data
                read_from_socket = recv(proTCPsocket, buff_rx_movie, row*col, MSG_WAITALL); //read UDP socket data into buffer
                //(after the first frame) delete old frame
                if (present_film == ON) { // make sure that screen is not taken
                    if (flag_first_frame_printout_pro) {
                        for (i = 0; i < row - 1; i++) { printf("\033[1A\033[2K\r"); }
                    }//delete a rows and move the 'cursor' up
                    //build buf
                    ptr_buf = 0;
                    for (i = 0; i < row - 1; i++) {
                        flag_EOL = 0;
                        for (j = 0; j < col; j++) {
                            if (!flag_EOL) {
                                cr = buff_rx_movie[ptr_buf];
                                if (cr == '\n') {
                                    flag_EOL = 1;
                                    printf("%c", buff_rx_movie[ptr_buf]);
                                }//end of line
                                if (!flag_EOL) { printf("%c", buff_rx_movie[ptr_buf]); }//line itself
                                ptr_buf = ptr_buf + 1;
                            }
                        }
                        if(!flag_EOL){printf("\n");}
                    }
                    flag_first_frame_printout_pro = 1;
                }
            }//end present_through_TCP
        }//end present_film
    }//end while 1
    close(myUDPsocket);
    return NULL;
}//end thread movie streamer

void *server_control_thread_function(void *v) {
    printf("\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");//print for user
    printf("Welcome to ASCII-Flix !\n");//print for user
    printf("\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");//print for user
    // declare inside variables
    struct sockaddr_in sock_struct;
    int i, t, c; //temp variables
    int read_from_socket, clientMenu, myTCPsocket;
    char line[100];

    /*SOCKET PROCESS*/
    //set sockaddr struct:
    sock_struct.sin_family = AF_INET;
    sock_struct.sin_port = htons(port_tcp_control);
    sock_struct.sin_addr.s_addr = ip_addr_server;
    // open a TCP socket - to the server
    myTCPsocket = socket(AF_INET, SOCK_STREAM, 0);
    //connect to sender
    c = connect(myTCPsocket, (struct sockaddr *) &sock_struct, sizeof(sock_struct));
    if (c == -1) {
        error_handler(0);
        return NULL;
    }
    printf("Connected to the server successfully!\n");

    // send a HELLO message
    if (send_msg(myTCPsocket, Hello, 0, 0, 0)) {
        if (!time_out(myTCPsocket)) { read_from_socket = recv(myTCPsocket, buff_rx, buffer_size, 0); }
        if (read_from_socket <= 0) { error_handler(4); }// err massage
        decode_ctrl_massage(buff_rx);
        register_to_a_new_movie_socket(0, 1, 0);//UDP station 0 first time
        flag_first_frame_printout = 0;
        present_film = ON;
    } else { error_handler(1); }

    while (!quit) {
        fgets(line, sizeof(line), stdin);
        present_film = OFF; // if user press enter -> movie stop
        fflush(stdin);
        printf("CSE AssciFlix Player:\n1. Change Movie\n2. Ask For Premium\n3. Return to watch\n4. Quit Program\n");
        fgets(line, sizeof(line), stdin);
        sscanf(line, "%d", &clientMenu);
        fflush(stdin);

        if (clientMenu == 1) {
            int station;
            printf("Please enter a station:  \n");
            fgets(line, sizeof(line), stdin);
            sscanf(line, "%d", &station);
            fflush(stdin);
            printf("this is station = %d\n", station);
            change_film(station, myTCPsocket);
            register_to_a_new_movie_socket(station, 1, 1);//station, UDP, not first time
            flag_first_frame_printout = 0;
            present_film = ON;
        }//Change Movie
        if (clientMenu == 2) { go_pro(myTCPsocket); }//Ask For Premium
        if (clientMenu == 3) {
            printf("Return to watch:  \n");
            flag_first_frame_printout = 0;
            present_film = ON;
        }//Return to watch
        if (clientMenu == 4) {
            printf("Quit Program:  \n");
            quit = 1;
        }//Quit Program
        if (clientMenu != 1 && clientMenu != 2 && clientMenu != 3 && clientMenu != 4) { printf("Wrong input.\n"); }
    }//end while !quit
    close(myTCPsocket);
    return NULL;
}//end thread server control

////~////~//// MAIN ////~////~////
int main(int argc, char **argv) {
    //--declare main variables
    pthread_t thread_id_movie_streamer, thread_id_server_control; //create thread variable
    int i;//temp holders
    //--Decode to variables input from command line
    ip_addr_server = inet_addr(argv[1]); // the server IP number
    port_tcp_control = atoi(argv[2]); // port number of the TCP control socket

    //-------------------------------------------------------------------
    //--Open a new thread: Movie presenter + Screen menu
    pthread_create(&thread_id_server_control, NULL, server_control_thread_function, NULL);
    //--Open a new thread: TCP control
    pthread_create(&thread_id_movie_streamer, NULL, movie_streamer_thread_function, NULL);

    //-------------------------------------------------------------------
    pthread_join(thread_id_movie_streamer, NULL);//block until thread is finished - stop bugs
    pthread_join(thread_id_server_control, NULL);//block until thread is finished - stop bugs
    return 0;
}//end main @ Ilan & Shir.
