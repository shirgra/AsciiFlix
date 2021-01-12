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

extern int errno;//todo noam
//--Global variables
// user input throughout the program
int quit = 0; //quit the program
// user initial input variables
in_addr_t ip_addr_server;
int port_tcp_control;
// buffer
uint8_t buff_tx[buffer_size];
uint8_t buff_rx[buffer_size];
// between threads
int mutex_io_film = 0;
int present_film = OFF;
int present_through_UDP = OFF;
int present_through_TCP = OFF;
int connect_to_a_new_film = OFF;
int connect_to_a_film_pro = OFF;
int myTCPsocket;
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
    int errnum;//todo noam
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
            printf("L 100 this is sending station: %x - %x\n", buff_tx[1], buff_tx[2]);
            //todo check station
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
    //send the buffer to the socket
    write_to_socket = send(socket, buff_tx, buffer_size, 0);
    if (write_to_socket == -1) {//todo fixme!
        errnum = errno;
        fprintf(stderr, "Value of errno: %d\n", errno);
        perror("Error printed by perror");
        fprintf(stderr, "Error opening file: %s\n", strerror(errnum));
    }
    if (write_to_socket > 0) { return 1; }//success
    else { return 0; }//fail
}

//decode massage
void decode_ctrl_massage(uint8_t buff_rx_temp[buffer_size]) {
    //declare variables
    int i; //temp
    err = 0;
    //check the message type
    replyType = (int) (buff_rx[0]);
    //switch cases
    switch (replyType) {
        case Welcome:
            numStations = ntohs(*(uint16_t *) &buff_rx_temp[1]);
            multicastGroup = ntohl(*(uint32_t *) &buff_rx_temp[3]);
            multicastGroup_current = multicastGroup;
            portNumber_UDP = ntohs(*(uint16_t *) &buff_rx_temp[7]);
            row = (uint8_t) buff_rx_temp[9];
            col = (uint8_t) buff_rx_temp[10];
            printf("Received Welcome message:\nNumber of stations: %x.\nM.C. group addr.: %x.\nPort number: %d.\n",
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
            // check for errors
            if (row <= 0 || col <= 0) { err = 1; }
            if (err) {//Disconnect
                printf("An error occurred during  decoding 'Announce message'\n");
                quit = 1;
            }
            break;

        case PermitPro:
            permit = buff_rx_temp[1];
            portNumber_Pro_TCP = ntohs(*(uint16_t *) &buff_rx_temp[2]);
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
            for (i = 0; i < replayStringSize; i++) { printf("%c", buff_rx_temp[1 + i]); }
            printf("\n");
            quit = 1;
            break;

        default:
            printf("Invalid RX message.\n");
            quit = 1;
    }
}

int time_out(void) {
    // Timeout and select definitions
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = timeout_time;
    //Initialize the file descriptor set. for timeout
    fd_set sock_set;
    FD_ZERO (&sock_set);
    FD_SET (myTCPsocket, &sock_set);
    int select_output;
    // start timeout
    select_output = select(myTCPsocket + 1, &sock_set, NULL, NULL, &timeout); // todo check if really work
    if (select_output == 0) {
        printf("Timeout has expired.\n");
        quit = 1;
        return 1;
    }// err massage
    else {
        return 0;
    }
}//end time_out


void change_film(type_theatre_to_connect) {
    present_film = OFF;
    int station;
    int read_from_socket;
    printf("Please enter a station:  \n");
    scanf("%d", &station);
    //Send AskFilm
    send_msg(myTCPsocket, AskFilm, 0x01, (uint16_t) (station), 0);
    if (!time_out()) { read_from_socket = recv(myTCPsocket, buff_rx, buffer_size, 0); }
    if (read_from_socket <= 0) { printf("Err in 'recv' function (Announce).\n"); }// err massage
    row = col = 0;//reset film variables
    decode_ctrl_massage(buff_rx);//get response
    // set film to present
    multicastGroup_current = multicastGroup + (uint32_t) (station);//todo fixme check the add
    type_theatre_to_connect = ON;//todo
    printf("Changed film to station: %d.\n", station);//todo debug
}

void faster(void) {
    present_film = OFF;
    float speedUp;
    int read_from_socket;
    printf("Please enter a number in range 0-100:  \n");
    scanf("%f", &speedUp); // 100% + 0%-100% for speed-up
    //Send AskFilm
    send_msg(myTCPsocket, SpeedUp, 0x03, 0, (uint8_t) (speedUp));
    // start timeout
    if (!time_out()) { read_from_socket = recv(myTCPsocket, buff_rx, buffer_size, 0); }
    if (read_from_socket <= 0) { printf("Err in 'recv' function (Ack).\n"); }// err massage
    decode_ctrl_massage(buff_rx);//get response
    // set film to present
    if (!mutex_io_film) {
        mutex_io_film = 1;
        present_film = ON;
        mutex_io_film = 0;
    }
    present_film = ON;
    printf("Speed up ACK receivd.\n");
}

void leave_pro(void) {
    present_film = OFF;
    int read_from_socket;
    //Send AskFilm
    send_msg(myTCPsocket, Release, 0x04, 0, 0);
    if (!time_out()) { read_from_socket = recv(myTCPsocket, buff_rx, buffer_size, 0); }
    if (read_from_socket <= 0) { printf("Err in 'recv' function (Announce).\n"); }// err massage
    printf("Left Pro service successfully.\n");
    //UDP return
    connect_to_a_new_film = ON;//todo check
}

void go_pro() {
    int read_from_socket, permitPro, menuPro;
    char enter;
    //Send GoPro
    send_msg(myTCPsocket, GoPro, 0x02, 0, 0);
    // start timeout
    if (!time_out()) { read_from_socket = recv(myTCPsocket, buff_rx, buffer_size, 0); }
    if (read_from_socket <= 0) { printf("Err in 'recv' function (GoPro).\n"); }// err massage
    //set premit pro
    decode_ctrl_massage(buff_rx);//get response permitPro
    permitPro = (int) (permit);
    if (permitPro) {
        connect_to_a_film_pro = ON;
        while (permitPro) {//-- PRO Menu --
            scanf("%c", &enter); //get enter
            if (!mutex_io_film) {
                mutex_io_film = 1;
                present_film = OFF; // if user press enter -> movie stop
                mutex_io_film = 0;
            }
            printf("Please enter a station:\n1. Change Movie\n2. Go Faster\n3. Leave Pro\n4. Return to watch\n5. Quit Program\n");
            scanf("%d", &menuPro);

            //Pro: Change Movie
            if (menuPro == 1) { change_film(connect_to_a_film_pro); }
            //Pro: Go Faster
            if (menuPro == 2) { faster(); }
            if (menuPro == 3)//Pro: Leave Pro
            {
                leave_pro();
                permitPro = 0;
            }
            if (menuPro == 4)//Pro: Return to watch
            {
                printf("Return to watch:  \n");
                // set film to present
                if (!mutex_io_film) {
                    mutex_io_film = 1;
                    present_film = ON;
                    mutex_io_film = 0;
                }

            }
            if (menuPro == 5)//Pro:Quit Program
            {
                printf("Quit Program:  \n");
                //Dissconect
                quit = 1;
                permitPro = 0;
            }
            if ((menuPro != 1) && (menuPro != 2) && (menuPro != 3) && (menuPro != 4) && (menuPro != 5)) {
                printf("try again...\n");
            }
        }//end while pro
    }//end if permit pro
    if (!permitPro) { printf("GoPro was not approved.\n"); }//todo check what happends next
}


//--MAIN functions
// Movie streaming thread function
void *movie_streamer_thread_function(void *v) {
    //function variables - local
    int myUDPsocket;
    int proTCPsocket;
    int expected_byte_num;
    int actual_byte_num;
    char buff_rx_movie[buffer_size];//buffer for data
    ssize_t read_from_socket;//read from socket
    struct ip_mreq mreq;//create multicast request
    struct sockaddr_in sock_struct_UDP;//create sockaddr struct:
    struct sockaddr_in sock_struct_TCP;//create sockaddr struct:
    int s;
    s = sizeof(sock_struct_UDP);
    int flag_mc_first_reg = 0;
    int flag_first_frame_printout = 0;
    int i, j; //temp holder

    //infinite loop - if we are requested to ask film
    while (!quit) {
        if (present_film == ON) {
            if (present_through_UDP == ON) {
                // start streaming user
                read_from_socket = recvfrom(myUDPsocket, buff_rx_movie, buffer_size, 0,
                                            (struct sockaddr *) &sock_struct_UDP,
                                            &s); //read UDP socket data into buffer
                //(after the first frame) delete old frame
                if (present_film == ON) { // make sure that screen is not taken
                    if (flag_first_frame_printout) {
                        for (i = 0; i < row; i++) { printf("\033[1A\033[2K\r"); }
                    }//delete a rows and move the 'cursor' up
                    //print frame
                    for (i = 0; i < row; i++) { for (j = 0; j < col; j++) { printf("%c", buff_rx_movie[i * col + j]); }}
                    flag_first_frame_printout = 1;
                }
            }//end present_through_UDP
            if (present_through_TCP == ON) {
                // start streaming user
                read_from_socket = recv(proTCPsocket, buff_rx_movie, buffer_size, 0); //read UDP socket data into buffer
                //(after the first frame) delete old frame
                if (present_film == ON) { // make sure that screen is not taken
                    if (flag_first_frame_printout) {
                        for (i = 0; i < row; i++) { printf("\033[1A\033[2K\r"); }
                    }//delete a rows and move the 'cursor' up
                    //print frame
                    for (i = 0; i < row; i++) { for (j = 0; j < col; j++) { printf("%c", buff_rx_movie[i * col + j]); }}
                    flag_first_frame_printout = 1;
                }
            }//end present_through_TCP
        }//end present_film
        if (connect_to_a_new_film == ON) {
            //present film name
            printf("    Presenting: ");
            for (i = 0; i < filmNameSize; i++) { printf("%c", filmName[i]); }
            printf("\n");
            if (flag_mc_first_reg) {
                //free resorces and quit M.C. old group
                setsockopt(myUDPsocket, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq));
                close(myUDPsocket);
                close(proTCPsocket);//todo check ok
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
            mreq.imr_multiaddr.s_addr = multicastGroup_current;    //takes uint32_bit
            mreq.imr_interface.s_addr = htonl(INADDR_ANY);
            //connect to m.c. group
            //printf("UDP listener is waiting...\n");
            setsockopt(myUDPsocket, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));///!!!
            //change mode
            if (mutex_io_film) {
                while (mutex_io_film) { ; };
                printf("L 277 in while mutex_io_film\n");
            }//todo del debug
            if (!mutex_io_film) {
                mutex_io_film = 1;
                connect_to_a_new_film = OFF;
                present_through_TCP = OFF;
                present_through_UDP = ON;
                present_film = ON;
                flag_first_frame_printout = 0;
                mutex_io_film = 0;
            }
            flag_mc_first_reg = 1; //set flag from the first time
        }// end connect_to_a_new_film
        if (connect_to_a_film_pro == ON) {
            //free resorces and quit M.C. old group
            setsockopt(myUDPsocket, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq));
            close(myUDPsocket);
            /*TCP SOCKET PROCESS*/
            //set sockaddr struct:
            sock_struct_TCP.sin_family = AF_INET;
            sock_struct_TCP.sin_port = htons(portNumber_Pro_TCP);
            sock_struct_TCP.sin_addr.s_addr = ip_addr_server;//todo check
            // open a TCP socket - to the server
            int proTCPsocket = socket(AF_INET, SOCK_STREAM, 0);
            //connect to sender
            int c = connect(proTCPsocket, (struct sockaddr *) &sock_struct_TCP, sizeof(sock_struct_TCP));
            if (c == -1) {
                printf("Sorry, could not connect to the server. Please make sure the server side is on.\n");
                quit = 1;
                return 0;
            }
            if (mutex_io_film) {
                while (mutex_io_film) { ; };
                printf("L 292 in while mutex_io_film\n");
            }//todo del debug
            if (!mutex_io_film) {
                mutex_io_film = 1;
                connect_to_a_film_pro = OFF;
                present_through_UDP = OFF;
                present_through_TCP = ON;
                present_film = ON;
                flag_first_frame_printout = 0;
                mutex_io_film = 0;
            }
        }//end connect_to_a_film_pro
    }//end while 1
    close(myUDPsocket);
    return 0;
}//end thread movie streamer

// TCP control thread function
void *server_control_thread_function(void *v) {
    printf("Welcome to ASCII-Flix !\n");//print for user
    // declare inside variables
    struct sockaddr_in sock_struct;
    int i, t; //temp variables
    int read_from_socket; //mark how much we read from the socket
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
    myTCPsocket = socket(AF_INET, SOCK_STREAM, 0);
    //connect to sender
    int c = connect(myTCPsocket, (struct sockaddr *) &sock_struct, sizeof(sock_struct));
    if (c == -1) {
        printf("Sorry, could not connect to the server. Please make sure the server side is on.\n");
        quit = 1;
        return 0;
    }

    // -- Connect to the server
    // send a HELLO message
    t = send_msg(myTCPsocket, Hello, 0, 0, 0);
    // start timeout
    if (!time_out()) { read_from_socket = recv(myTCPsocket, buff_rx, buffer_size, 0); }
    if (read_from_socket <= 0) { printf("Err in 'recv' function (Welcome).\n"); }// err massage
    // if receive WELCOME - read from socket
    decode_ctrl_massage(buff_rx);

    // get first film - mutex
    if (!mutex_io_film) {
        mutex_io_film = 1;
        connect_to_a_new_film = ON; // after present the movie
        mutex_io_film = 0;
    }
//            sleep(1);

    while (!quit) {//-- Menu --
        scanf("%c", &enter); //get enter
        if (!mutex_io_film) {
            mutex_io_film = 1;
            present_film = OFF; // if user press enter -> movie stop
            mutex_io_film = 0;
        }
        printf("CSE AssciFlix Player:\n1. Change Movie\n2. Ask For Premium\n3. Return to watch\n4. Quit Program\n");
        scanf("%d", &clientMenu);
        //Change Movie
        if (clientMenu == 1) { change_film(connect_to_a_new_film); }
        //Ask For Premium
        if (clientMenu == 2) { go_pro(); }
        //Return to watch
        if (clientMenu == 3) {
            printf("Return to watch:  \n");
                present_film = ON;
        }
        if (clientMenu == 4)//Quit Program
        {
            printf("Quit Program:  \n");
            quit = 1;
            //Dissconect from sockets
            close(myTCPsocket);
        }
        if ((clientMenu != 1) && (clientMenu != 2) && (clientMenu != 3) && (clientMenu != 4)) {
            printf("try again...\n");
        }
    }//end while 1
    close(myTCPsocket);
    return 0;
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

    //-------------------------------------------------------------------
    pthread_join(thread_id_movie_streamer, NULL);//block until thread is finished - stop bugs
    pthread_join(thread_id_server_control, NULL);//block until thread is finished - stop bugs

    return 0;
}//end main
// @ Ilan & Shir.