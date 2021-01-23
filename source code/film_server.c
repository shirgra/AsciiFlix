/*====================================================================================
  ====================================================================================
  Final project in Lab session of CCS2 course.
  Written by Ilan Klein & Shir Granit.


				----------  Ascii-Film App:  Server side  ----------


	compile with gcc server.c -o film_server -lpthread

	run with ./film_server <tcp control port> <tcp pro port> <multicast ip adr>
							<udp port> <file1> <file2> <file3> <file4> ...

    e.g. ./film_server 5555 6666 224.1.1.234 7777 sw1.txt Intro.txt rick.txt bomb.txt

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
#include <fcntl.h> //read files
#include <errno.h> //err
//#include <sys/types.h>
//#include <ifaddrs.h>
//#include <sys/time.h> //for timer
//#include <netdb.h> //to get my ip
//#include <sys/ioctl.h>
//#include <netinet/in.h>
//#include <net/if.h>

//--Defines:
#define client_max_size 50
#define client_pro_size 2
#define movies_max_size 10
// general
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
extern int errno;                           //mark error
int quit = 0;                               //for when the user wants to quit
int client_tracker[client_max_size];        //clients tracker: -3 = no client ,default at restart, -2 = no client, any int = client
int pro_client_tracker[client_pro_size] = {0};
int pro_function_quit_flag[client_pro_size + 1];//mark flag for pro function - active or not
int mutex_clients = 0;                      //mutex lock access to the arrays: 0 = available, 1 = taken by another thread
int mark;                                   //mark for new thread witch one is it
//Movies
char *files_name[movies_max_size];          //this array is to keep track on the movies we screen
int movies_dim[movies_max_size][2];         //keep movies dimensions
int movies_fd[movies_max_size];             //this array keeps the fd int number of the movies txt files
int no_files;                               //mark the number of files an input part in main
int movie_sockets[movies_max_size];         //keep UDPs fd numbers
int udp_socket_thread_ids[movies_max_size]; //transfer int for thread function
struct sockaddr_in movie_sock_struct[movies_max_size];
//Input from user
int port_no_tcp_ctrl;                       //port number of the welcome TCP
int port_no_tcp_pro;                        //port number of the pro TCP
int port_no_udp;                            //port number of the broadcasting the movies
in_addr_t mc_addr;                          //multicast addr
//help functions variables
struct premium_args {
    int client_id;
    int movie_id;
    int speedup;
    int socket_info;
};                  //pass arguments for thread function - pro
int pos_ = 0;                               //for movie sending threads
int touch_pos = 0;
int current_movie_tcp = 0;                  //mark the current movie presented

//~~ Help functions
int join_clients(newClient_TCPsocket) {
    int flag, i;//temps
    if (mutex_clients) { while (mutex_clients) { ; }; }//mutex
    mutex_clients = 1;
    flag = 0;
    for (i = 0; i < client_max_size; i++) {
        if (((client_tracker[i] == -3) || (client_tracker[i] == -2)) && (!flag)) {
            client_tracker[i] = newClient_TCPsocket;
            mark = i;
            flag = ON;
        }
    }
    mutex_clients = 0;
    if (flag) { return 1; } //success
    else { return 0; } //didnt success
}//join_clients

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
        //quit = ON;
        return ON;
    }// err massage
    else {
        return OFF;
    }
}//time_out (control)

void timer(long rate) {
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = (long) (1e9 * rate / 24);
    nanosleep(&ts, NULL);
}//timer (movie)

int sendInvalid(int socket, char *string) {
    //function vars
    int i;
    uint8_t buff_tx[buffer_size];
    for (i = 0; i < buffer_size; i++) { buff_tx[i] = 0; }
    int write_to_socket;
    uint8_t len = (uint8_t) (strlen(string));
    //built messege
    buff_tx[0] = 0x04;
    buff_tx[1] = len;
    for (i = 0; i < len; i++) { buff_tx[i + 2] = string[i]; }
    write_to_socket = send(socket, buff_tx, buffer_size, 0);
    if (write_to_socket == -1) {
        fprintf(stderr, "Value of errno: %d\n", errno);
        perror("Error printed by perror");
    }
    if (write_to_socket > 0) { return 1; }//success
    else { return 0; }//fail
}//sendInvalid

int send_ctrl(int sock, int type, int file_mark, uint8_t temp_8b) {
    //function vars
    int i, t;
    uint8_t buff_tx[buffer_size];
    int write_to_socket;
    //switch cases
    switch (type) {
        case Welcome:
            buff_tx[0] = 0x00;
            *(uint16_t *) &buff_tx[1] = htons((no_files));//1 2
            *(uint32_t *) &buff_tx[3] = htonl(mc_addr);//3 4 5 6
            *(uint16_t *) &buff_tx[7] = htons((port_no_udp));//7 8
            buff_tx[9] = (uint8_t) movies_dim[0][0];
            buff_tx[10] = (uint8_t) movies_dim[0][1];
            printf("Sent a Welcome message.\n");
            break;
        case Announce:
            buff_tx[0] = 0x01;
            buff_tx[1] = (uint8_t) movies_dim[file_mark][0];
            buff_tx[2] = (uint8_t) movies_dim[file_mark][1];
            buff_tx[3] = t = (uint8_t) (strlen(files_name[file_mark]));
            for (i = 0; i < t; i++) { buff_tx[4 + i] = (uint8_t) files_name[file_mark][i]; }
            printf("Sent an Announce message.\n");
            break;
        case PermitPro:
            buff_tx[0] = 0x02;
            if (temp_8b) {
                buff_tx[1] = 0x01;
                *(uint16_t *) &buff_tx[2] = htons((port_no_tcp_pro));//7 8
            } else {
                buff_tx[1] = 0;
                buff_tx[2] = 0;
            }
            printf("Sent a PermitPro message.\n");
            break;
        case Ack:
            buff_tx[0] = 0x03;
            buff_tx[1] = temp_8b;
            printf("Sent ACK message.\n");
            break;
    }
    //send the buffer to the socket
    write_to_socket = send(sock, buff_tx, buffer_size, 0);
    if (write_to_socket == -1) { printf("Error in sending. did not sent message.\n"); }
    if (write_to_socket > 0) { return 1; }//success
    else { return 0; }//fail
}//send_ctrl

int verify(int socket, int mssg_expected, uint8_t buff[buffer_size]) {
    int err = 0;//1=not in format; 2= out of range
    uint16_t temp_16b;
    switch (mssg_expected) {
        case Hello:
            if ((buff[0] != 0x00) || (buff[1] != 0x00) || (buff[2] != 0x00)) { err = 1; };
            break;
        case AskFilm:
            if (buff[0] != 0x01) { err = 1; };
            temp_16b = htons(*(uint16_t *) &buff[1]);
            if ((int) temp_16b > no_files) { err = 2; }
            break;
        case GoPro:
            if ((buff[0] != 0x02) || (buff[1] != 0x00) || (buff[2] != 0x00)) { err = 1; };
            break;
        case SpeedUp:
            if ((buff[0] != 0x03) || (buff[1] == 0x00)) { err = 1; };
            break;
        case Release:
            if ((buff[0] != 0x04) || (buff[1] != 0x00) || (buff[2] != 0x00)) { err = 1; };
            break;
    }
    switch (err) {
        case 0:
            return 1;
        case 1:
            printf("Err. in verification of a message\n");
            sendInvalid(socket,
                        "---Connection terminated by the server.\n---Message from server: error in verification of a message.");
            return 0;
        case 2:
            printf("Err. in verification of a message - out of range values.\n");
            sendInvalid(socket,
                        "---Connection terminated by the server.\n---Message from server: error in range of values.");
            return 0;
    }
}//verify

int check_pro_permit(void) {
    int i;
    int flag = 0, m = 0;
    //mutex get client data
    if (mutex_clients) { while (mutex_clients) { ; }; }
    mutex_clients = 1;//mutex for touching the clients
    for (i = 1; i <= client_pro_size; i++) {
        if (!pro_client_tracker[i] && !flag) {
            flag = 1;
            m = i;
        }
    }
    mutex_clients = 0;
    if (flag) {
        pro_client_tracker[m] = 1;
        return m;
    } else { return 0; }
}//check_pro_permit

void *pro_movie_streamer_thread_function(void *args) {
    struct premium_args *pro_args = (struct premium_args *) args;
    char *endptr;
    char line[256];
    long rate, r;
    int place = pro_args->movie_id;
    ssize_t write_to_socket;//read from socket
    //get local vars: mutex get client data
    if (mutex_clients) { while (mutex_clients) { ; }; }
    mutex_clients = 1;//mutex for touching the clients //fixme
    int self_row = movies_dim[place][0];
    int self_col = movies_dim[place][1];
    int self_fd = movies_fd[place];
    char *self_name_file = files_name[place];
    struct sockaddr_in self_sock_struct = movie_sock_struct[place];
    mutex_clients = 0;
    char buf[buffer_size], buf_5[5];
    char buf_rate[2];
    //start film from beginning
    self_fd = open(self_name_file, O_RDONLY);
    if (self_fd == -1) {
        printf("ERR- can not open %s file\n", self_name_file);
        return NULL;
    }
    //read dimentions
    read(self_fd, buf_5, 5);//already has them

    int ptr_buf, w1, w2, i;
    while (!pro_function_quit_flag[pro_args->client_id]) {
        w1 = read(self_fd, buf_rate, 2);//read first line of frame
        if (w1 == 2) {
            if (buf_rate[1] == '\n') {
                //read row lines from file
                buf_rate[1] = 0;//delete end
                rate = strtol(buf_rate, &endptr, 10);//set rate
                rate = rate / (0.01 * (pro_args->speedup) + 1);
//                printf("rate: %ld \n", rate);
            } else {
                rate = strtol(buf_rate, &endptr, 10);//set rate
                read(self_fd, buf_rate, 1);
                rate = rate * (0.01 * (pro_args->speedup) + 1);
//                printf("rate: %ld \n", rate);
            }//cases if 1 char or 2 in rate
            ptr_buf = 0;
            char cr[1];
            int flag_EOL, j, flag_dont_send = 0;
            for (i = 0; i < self_row - 1; i++) {
                flag_EOL = 0;
                for (j = 0; j < self_col; j++) {
                    if (!flag_EOL) {
                        w2 = read(self_fd, cr, 1);
                        if (w2 == 1) {
                            if (i == 0 && cr[0] == '\n') {
                                flag_EOL = 1;
                                buf[ptr_buf] = '\n';
                            }//handle first char is \n
                            if (cr[0] == '\n') {
                                flag_EOL = 1;
                                buf[ptr_buf] = '\n';
                            }//end of line
                            if (!flag_EOL) { buf[ptr_buf] = cr[0]; }//line itself
                            ptr_buf = ptr_buf + 1;
//                            printf("%c", cr[0]);
                        }//read 1 char
                        else {
                            flag_dont_send = 1;
                            flag_EOL = 1;
                        }//end of file
                    }//if not end of line
                }
            }//build buf
            if (flag_dont_send) {
                close(self_fd);
                self_fd = open(self_name_file, O_RDONLY);
                if (self_fd == -1) {
                    printf("ERR- can not open %s file\n", self_name_file);
                    return NULL;
                }
            }//end of file
            else {
                //write to socket
                send(pro_args->socket_info, buf, self_col * self_row, 0);
                //timer - sleep
                timer(rate);
            }//send frame !

        } else {
            close(self_fd);
            self_fd = open(self_name_file, O_RDONLY);
            if (self_fd == -1) {
                printf("ERR- can not open %s file\n", self_name_file);
                return NULL;
            }//err in file opening
        }//end of file
    }
    printf("A client has left the PRO services successfully\n");
    return NULL;
}//pro_movie_streamer_thread_function

void activate_response_pro(int sock, int pro_id) {
    int pro_client_socket, trans_client_socket, b, l, c;
    struct sockaddr_in welcome_sock_struct_pro, client_sock_struct_pro;//creat sockaddr struct:
    int quit_pro = 0;
    c = sizeof(struct sockaddr_in);
    int station_req;
    int new_req = 0;
    pthread_t thread_pro_movie_streamer;

    /*TCP SOCKET PROCESS*/
    //open a TCP socket
    pro_client_socket = socket(AF_INET, SOCK_STREAM, 0);
    //set vars to sock struct - srv:
    welcome_sock_struct_pro.sin_family = AF_INET;
    welcome_sock_struct_pro.sin_port = htons(port_no_tcp_pro);
    welcome_sock_struct_pro.sin_addr.s_addr = INADDR_ANY;
    //bind socket
    b = bind(pro_client_socket, (struct sockaddr *) &welcome_sock_struct_pro, sizeof(welcome_sock_struct_pro));
    while (b == -1) {
        printf("Was not able to bind. Trying again...\n");
        sleep(3);
        b = bind(pro_client_socket, (struct sockaddr *) &welcome_sock_struct_pro, sizeof(welcome_sock_struct_pro));
    } //try to bind again
    l = listen(pro_client_socket, SOMAXCONN);//wait for a connection - listen
    if (!l) {//listening...
        printf("Listening PRO...\n");
        trans_client_socket = accept(pro_client_socket, (struct sockaddr *) &client_sock_struct_pro, (socklen_t *) &c);
    } else { quit_pro = 1; }
    printf("Connected to a TCP PRO socket\n");

    //-------------------------------------------------------------------
    //--Open a new thread: Transmitting movies
    struct premium_args args;
    struct premium_args *args_ptr = &args;
    args_ptr->client_id = pro_id;
    args_ptr->movie_id = current_movie_tcp;
    args_ptr->socket_info = trans_client_socket;
    args_ptr->speedup = 0; //default
    pro_function_quit_flag[pro_id] = 0;//mark flag when to stop activate function
    pthread_create(&thread_pro_movie_streamer, NULL, pro_movie_streamer_thread_function, (void *) args_ptr);

    //-------->> Infinite loop:
    while (!quit_pro) {
        int read_from_socket;
        uint8_t buff[buffer_size];
        read_from_socket = recv(sock, buff, buffer_size, 0); //read UDP socket data into buffer
        if (buff[0] == 0x01) {
            if (verify(sock, AskFilm, buff)) {
                uint16_t station_r_b = ntohs(*(uint16_t *) &buff[1]);
                station_req = (int) (station_r_b);
                printf("Received AskFilm for station %d.\n", station_req);
                send_ctrl(sock, Announce, station_req, 0);
                pro_function_quit_flag[pro_id] = 1;//stop thread
                pthread_join(thread_pro_movie_streamer, NULL);//block until thread is finished - stop bugs
                //--Open a new thread: Transmitting movies
                args_ptr->client_id = pro_id;
                args_ptr->movie_id = station_req;
                current_movie_tcp = station_req;
                args_ptr->socket_info = trans_client_socket;
                args_ptr->speedup = 0; //default
                pro_function_quit_flag[pro_id] = 0;//mark flag when to stop activate function
                pthread_create(&thread_pro_movie_streamer, NULL, pro_movie_streamer_thread_function, (void *) args_ptr);
            }
        }//change film
        if (buff[0] == 0x03) {
            send_ctrl(sock, Ack, 0, 0x03);
            printf("Received SpeedUp request for %d%%.\n.", (int) buff[1]);
            pro_function_quit_flag[pro_id] = 1;//stop thread
            pthread_join(thread_pro_movie_streamer, NULL);//block until thread is finished - stop bugs
            //--Open a new thread: Transmitting movies
            args_ptr->client_id = pro_id;
            args_ptr->movie_id = current_movie_tcp;
            args_ptr->socket_info = trans_client_socket;
            args_ptr->speedup = (int) buff[1]; //speed up by request
            pro_function_quit_flag[pro_id] = 0;//mark flag when to stop activate function
            pthread_create(&thread_pro_movie_streamer, NULL, pro_movie_streamer_thread_function, (void *) args_ptr);
        }//speedUP
        if (buff[0] == 0x04) {
            printf("Received Release.\n");
            send_ctrl(sock, Ack, 0, 0x03);
            close(pro_client_socket);
            pro_function_quit_flag[pro_id] = 1;//stop thread
            pro_client_tracker[pro_id] = 0;
            pthread_join(thread_pro_movie_streamer, NULL);//block until thread is finished - stop bugs
            quit_pro = 1;
        }//quit PRO
    }//end while ! quit pro
}//react pro - Menu Pro

void activate_response(int sock) {
    int read_from_socket;
    uint8_t buff[buffer_size];
    read_from_socket = recv(sock, buff, buffer_size, 0); //read UDP socket data into buffer
    if (read_from_socket > 0) {
        if (buff[0] == 0x01) {
            if (verify(sock, AskFilm, buff)) {
                uint16_t station_r_b = ntohs(*(uint16_t *) &buff[1]);
                int station_r = (int) (station_r_b);
                printf("Received AskFilm for station %d\n", station_r);
                send_ctrl(sock, Announce, station_r, 0);
                current_movie_tcp = station_r;
            }
        }//change film
        if (buff[0] == 0x02) {
            printf("Received GoPro...");
            if (verify(sock, GoPro, buff)) {
                int id_pro = check_pro_permit();
                if (id_pro != 0) {
                    printf("Permit.\n");
                    send_ctrl(sock, GoPro, 0, 0x01);
                    activate_response_pro(sock, id_pro);
                } else {
                    send_ctrl(sock, GoPro, 0, 0);
                    printf("Did not Permit.\n");
                }
            }
        }//goPro
    } else { client_tracker[mark] = -2; }
}//activate_response - Menu

//~~ Thread functions:
void *send_film_thread_function(void *arg) {
//    printf("send_film_thread_function\n");
    int i;
    if (touch_pos == 0) {
        touch_pos = 1;
        i = pos_;
        pos_ = pos_ + 1;
        touch_pos = 0;
    } else {
        printf("Unable to touch\n");
        i = 0;
    }//get id i
    char c[1], buf_rate[2];
    char *endptr;
    int r, w1, w2;
    long rate;
    ssize_t write_to_socket;//read from socket
    //get local vars: mutex get client data
    if (mutex_clients) { while (mutex_clients) { ; }; }
    mutex_clients = 1;//mutex for touching the clients //fixme
    int self_row = movies_dim[i][0];
    int self_col = movies_dim[i][1];
    int self_fd = movies_fd[i];
    int self_sock = movie_sockets[i];
    char *self_name_file = files_name[i];
    mutex_clients = 0;
    struct sockaddr_in self_sock_struct = movie_sock_struct[i];
    char buf[self_col * self_row];
    int ptr_buf;
    while (!quit) {
        w1 = read(self_fd, buf_rate, 2);//read first line of frame
        if (w1 == 2) {
            if (buf_rate[1] == '\n') {
                //read row lines from file
                buf_rate[1] = 0;//delete end
                rate = strtol(buf_rate, &endptr, 10);//set rate
            } else {
                rate = strtol(buf_rate, &endptr, 10);//set rate
                read(self_fd, buf_rate, 1);
            }//cases if 1 char or 2 in rate
            ptr_buf = 0;
            char cr[1];
            int flag_EOL, j, flag_dont_send = 0;
            for (i = 0; i < self_row - 1; i++) {
                flag_EOL = 0;
                for (j = 0; j < self_col; j++) {
                    if (!flag_EOL) {
                        w2 = read(self_fd, cr, 1);
                        if (w2 == 1) {
                            if (i == 0 && cr[0] == '\n') {
                                flag_EOL = 1;
                                buf[ptr_buf] = '\n';
                            }//handle first char is \n
                            if (cr[0] == '\n') {
                                flag_EOL = 1;
                                buf[ptr_buf] = '\n';
                            }//end of line
                            if (!flag_EOL) { buf[ptr_buf] = cr[0]; }//line itself
                            ptr_buf = ptr_buf + 1;
                        }//read 1 char
                        else {
                            flag_dont_send = 1;
                            flag_EOL = 1;
                        }//end of file
                    }//if not end of line
                }
            }//build buf
            if (flag_dont_send) {
                close(self_fd);
                self_fd = open(self_name_file, O_RDONLY);
                if (self_fd == -1) {
                    printf("ERR- can not open %s file\n", self_name_file);
                    return NULL;
                }
            }//end of file
            else {
                //write to socket
                sendto(self_sock, buf, self_col * self_row, 0, (struct sockaddr *) &self_sock_struct,
                       sizeof(self_sock_struct));
                //timer - sleep
                timer(rate);
            }//send frame !

        } else {
            close(self_fd);
            self_fd = open(self_name_file, O_RDONLY);
            if (self_fd == -1) {
                printf("ERR- can not open %s file\n", self_name_file);
                return NULL;
            }//err in file opening
        }//end of file
    }
//    printf("quit in send_film_thread_function\n");
    return NULL;
}//end send_film_thread_function- help for movie thread

void *movie_streamer_thread_function(void *v) {
    //variables declarations
    int myUDPsocket, i;//temp keeper
    u_char ttl = 32;//temp to set properties to M.C.
    struct sockaddr_in sock_struct;//creat sockaddr struct:
    struct ip_mreq mreq;//create multicast request
    int write_to_socket;
    char buf[buffer_size];
    pthread_t movie_threads[no_files];

    /* UDP SOCKET PROCESS */
    for (i = 0; i < no_files; i++) { //to every movie:
        //open a UDP socket
        myUDPsocket = socket(AF_INET, SOCK_DGRAM, 0);
        //set socket properties
        sock_struct.sin_family = AF_INET;
        sock_struct.sin_port = htons(port_no_udp);
        sock_struct.sin_addr.s_addr = mc_addr + (uint32_t) i;
        //multicast definitions
        setsockopt(myUDPsocket, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));
        //append to the array movie_sockets
        movie_sockets[i] = myUDPsocket;
        movie_sock_struct[i] = sock_struct;
        //create a transmission thread for this movie
        pthread_create(&movie_threads[i], NULL, send_film_thread_function, (void *) (udp_socket_thread_ids + i));
    }
    //hold untill threads are dead
    for (i = 0; i < no_files; i++) { pthread_join(movie_threads[i], NULL); }
    return NULL;
}//end movie_streamer_thread_function

void *active_client_thread_function(void *v) {
    //variables
    int read_from_socket, t, self_mark, self_id;
    uint8_t buff_rx[buffer_size];
    if (mutex_clients) { while (mutex_clients) { ; }; }//mutex read client data
    mutex_clients = ON;//mutex for touching the clients
    self_mark = mark;
    mutex_clients = OFF;//mutex for touching the clients
    self_id = client_tracker[self_mark];
    //receive hello
    if (!time_out(self_id)) {
        read_from_socket = recv(self_id, buff_rx, buffer_size, 0);
        if (read_from_socket <= 0) { printf("Err in 'recv' function (Hello).\n"); }// err massage
    }//receive HELLO message correctly
    //write to socket Welcome
    if (verify(self_id, Hello, buff_rx)) { send_ctrl(self_id, Welcome, 0, 0); }
    while (client_tracker[self_mark] != -2) { activate_response(self_id); }
    return NULL;
}//end active_client_thread_function

void *listen_to_clients_thread_function(void *v) {
    //variables
    pthread_t thread_id_new_client;
    int flag;//flags that a new cell has been fount in the new clients array
    int b, l, c, i;//temp holders
    int welcomeTCPsocket;
    int newClient_TCPsocket;
    struct sockaddr_in welcome_sock_struct;//creat sockaddr struct:
    struct sockaddr_in client_sock_struct;//socket for client
    c = sizeof(struct sockaddr_in);

    /*SOCKET PROCESS*/
    //open a TCP socket --Open a Welcome socket (TCP control with clients)
    welcomeTCPsocket = socket(AF_INET, SOCK_STREAM, 0);
    welcome_sock_struct.sin_family = AF_INET;
    welcome_sock_struct.sin_port = htons(port_no_tcp_ctrl);
    welcome_sock_struct.sin_addr.s_addr = INADDR_ANY;
    //bind socket
    b = bind(welcomeTCPsocket, (struct sockaddr *) &welcome_sock_struct, sizeof(welcome_sock_struct));
    while (b == -1) {
        printf("Was not able to bind. Trying again...\n");
        sleep(3);
        b = bind(welcomeTCPsocket, (struct sockaddr *) &welcome_sock_struct, sizeof(welcome_sock_struct));
    } //try to bind again

    while (!quit) {
        //wait for a connection - listen
        l = listen(welcomeTCPsocket, SOMAXCONN);
        if (!l) {//listening...
            //STOPS UNTIL A NEW CLIENT -->> accept connection from an incoming client
            newClient_TCPsocket = accept(welcomeTCPsocket, (struct sockaddr *) &client_sock_struct, (socklen_t *) &c);
            if (newClient_TCPsocket != -1) {
                printf("A new client has joined AsciiFlix! Yey!\n");
                //attach new client to a new thread
                if (join_clients(newClient_TCPsocket)) {
                    //send client to be taken care of in another thread
                    pthread_create(&thread_id_new_client, NULL, active_client_thread_function, NULL);
                }
            } else { printf("ERR Accepting new client. Try again...\n"); }
        } else { printf("ERR listening. Try again...\n"); }
    }
    close(welcomeTCPsocket);
    return NULL;
}//end listen_to_clients_thread_function

void *user_quit_thread_function(void *v) {
    char input_char, enter; //input for 'quit' from user
    while (!quit) {
        int i;
        //quit option - input from user
        printf("Press q + Enter to quit the program at any time\n");
        scanf("%c", &input_char);
        scanf("%c", &enter);
        fflush(stdin);
        if (input_char == 'q') {
            quit = 1;
            printf("Goodbye!\n");
            for (i = 0; i < client_max_size; i++) {
                if (((client_tracker[i] != -3) && (client_tracker[i] != -2))) {
                    close(client_tracker[i]);
                }
            }
        }    // check input
    }
    return 0;
}//end user_quit_thread_function

////~////~//// MAIN ////~////~////
int main(int argc, char **argv) {
    // main variables declarations
    int i; //temp variable
    pthread_t thread_id_movie_streamer, thread_id_welcome_socket, thread_id_user_quit; //create thread variable

    //~~ First activation:
    //--declare main variables
    if (ON) {
        //Decode to variables input from command line - GLOBAL
        port_no_tcp_ctrl = atoi(argv[1]); //port number of the welcome TCP
        port_no_tcp_pro = atoi(argv[2]); //port number of the pro TCP
        port_no_udp = atoi(argv[4]); //port number of the broadcasting the movies
        mc_addr = inet_addr(argv[3]); //multicast addr
        no_files = argc - 5; //number of files to screen
        for (i = 0; i < client_max_size; i++) { client_tracker[i] = -3; } //reset client array
        for (i = 0; i < no_files; i++) { files_name[i] = argv[5 + i]; } //input files names in name array
        //prep work for thread- open files to read
        int fd, r;
        char buf[5];
        for (i = 0; i < no_files; i++) {
            fd = open(files_name[i], O_RDONLY);
            if (fd == -1) {
                printf("ERR- can not open %s file\n", files_name[i]);
                return 0;
            }
            movies_fd[i] = fd; //save fd
            r = read(fd, buf, 5); // const format "XX XX"
            movies_dim[i][0] = ((int) (buf[0] - '0')) * 10 + (int) (buf[1] - '0');
            movies_dim[i][1] = ((int) (buf[3] - '0')) * 10 + (int) (buf[4] - '0');
        }
    }

    //User inform:
    printf("\nHello user and welcome to AsciiFlix SERVER side.\n");
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    printf("Technical information:\nTCP control port number: %d\n", port_no_tcp_ctrl);
    printf("UDP movie streamer port number: %d\nUDP movie streamer multicast address: %s\n", port_no_udp, argv[3]);
    printf("TCP pro user port number: %d\n", port_no_tcp_pro);
    printf("Today movies are: ");
    for (i = 0; i < no_files; i++) { printf("%s ", files_name[i]); }
    printf("\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");

    //-------------------------------------------------------------------
    //--Open a new thread: Transmitting movies
    pthread_create(&thread_id_movie_streamer, NULL, movie_streamer_thread_function, NULL);
    //--Open a new thread: listening for new clients
    pthread_create(&thread_id_welcome_socket, NULL, listen_to_clients_thread_function, NULL);
    //--Open a new thread: listening for new clients
    pthread_create(&thread_id_user_quit, NULL, user_quit_thread_function, NULL);

    //-------------------------------------------------------------------
    while (!quit);
    sleep(1);
    return 0;//just in case
}//end main @ Ilan & Shir.