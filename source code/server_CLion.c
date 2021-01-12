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
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <fcntl.h> //read files
#include <ifaddrs.h>
#include <errno.h> //err
#include <sys/time.h> //for timer

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
extern int errno;//todo noam
int quit = 0;//for when the user wants to quit
/* Array of clients tracker = size clients_max_size
 * -3 = no client ,default at restart
 * -2 = no client - mark that this cell is not the active ending of the clients list
 * any positive int = client is connected and online*/
int client_tracker[client_max_size];
int pro_client_tracker[client_pro_size] = {0};
int pro_thread_quit_flag[client_pro_size + 1];
int mutex_clients = 0; //mutex lock access to the arrays: 0 = available, 1 = taken by another thread
int mark;//mark for new thread witch one is it
//Movies
char *files_name[movies_max_size]; //this array is to keep track on the movies we screen
int movies_dim[movies_max_size][2];
int movies_fd[movies_max_size]; //this array keeps the fd int number of the movies txt files
int movies_bit_flag[movies_max_size]; //this array is to keep track of movies that need to switch frames
int no_files;//mark the number of files an input part in main
int movie_sockets[movies_max_size];//keep UDPs fd numbers
struct sockaddr_in movie_sock_struct[movies_max_size];
//Input from user
int port_no_tcp_ctrl; //port number of the welcome TCP
int port_no_tcp_pro; //port number of the pro TCP
int port_no_udp; //port number of the broadcasting the movies
in_addr_t mc_addr;//multicast addr
uint8_t row_base;
uint8_t col_base;
int LINE_MAX = 1000;
int udp_socket_thread_ids[movies_max_size];
struct premium_args {
    int client_id;
    int movie_id;
    int speedup;
    int socket_info;
};
int touch_pos = 0;
int pos = 0;

//--Side Help functions
//fixme! get self id
//int get_IP(void){//return the user IP address
//    struct ifaddrs *id;
//    int val;
//    val = getifaddrs(&id);
//    printf("Network Interface Name :- %s\n",id->ifa_name);
//    printf("Network Address of %s :- %d\n",id->ifa_name,id->ifa_addr);
//    printf("Network Data :- %d \n",id->ifa_data);
//    printf("Socket Data : -%c\n",id->ifa_addr->sa_data);
//    return id->ifa_addr;
//}


//side help functions

void timer(int rate) {
    long time = 1000000000 * rate / 24;
    nanosleep((const struct timespec[]) {{0, time}}, NULL);
}

int check_pro_permit(void) {
    int i;
    int flag, mark = 0;
    //mutex get client data
    if (mutex_clients) { while (mutex_clients) { ; }; }
    mutex_clients = 1;//mutex for touching the clients //fixme
    for (i = 1; i < client_pro_size; i++) {
        if (!pro_client_tracker[i] && !flag) {
            flag = 1;
            mark = i;
        }
    }
    mutex_clients = 0;
    if (flag) {
        pro_client_tracker[mark] = 1;
        return mark;
    } else { return 0; }
}

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
    select_output = select(sock + 1, &sock_set, NULL, NULL, &timeout); // todo check if really work
    if (select_output == 0) {
        printf("Timeout has expired.\n");
        quit = 1;
        return 1;
    }// err massage
    else {
        return 0;
    }
}//end time_out

int sendInvalid(int socket, char *string) {
    //function vars
    int errnum, i;
    uint8_t buff_tx[buffer_size];
    uint16_t temp_16b;
    for (i = 0; i < buffer_size; i++) { buff_tx[i] = 0; }
    int write_to_socket;
    uint8_t len = (uint8_t) (strlen(string));
    //built messege
    buff_tx[0] = 0x04;
    buff_tx[1] = len;
    for (i = 0; i < len; i++) { buff_tx[i + 2] = string[i]; }
    write_to_socket = send(socket, buff_tx, buffer_size, 0);
    if (write_to_socket == -1) {//todo fixme!
        errnum = errno;
        fprintf(stderr, "Value of errno: %d\n", errno);
        perror("Error printed by perror");
    }
    if (write_to_socket > 0) { return 1; }//success
    else { return 0; }//fail
}

int send_ctrl(int sock, int type, int file_mark, uint8_t temp_8b, uint16_t temp_16b, uint32_t temp_32b) {
    //function vars
    int errnum, i, t;
    uint8_t buff_tx[buffer_size];
    for (i = 0; i < buffer_size; i++) { buff_tx[i] = 0; }
    int write_to_socket;
    //switch cases
    switch (type) {
        case Welcome:
            temp_16b = (uint16_t) (no_files);
            buff_tx[0] = 0x00;
            *(uint16_t *) &buff_tx[1] = htons((no_files));//1 2
            *(uint32_t *) &buff_tx[3] = mc_addr;//3 4 5 6
            *(uint16_t *) &buff_tx[7] = htons((port_no_udp));//7 8
            buff_tx[9] = (uint8_t) movies_dim[0][0];
            buff_tx[10] = (uint8_t) movies_dim[0][1];
            break;
        case Announce:
            buff_tx[0] = 0x01;//todo
            buff_tx[1] = (uint8_t) movies_dim[file_mark][0];
            buff_tx[2] = (uint8_t) movies_dim[file_mark][1];
            buff_tx[3] = t = (uint8_t) (strlen(files_name[file_mark]));
            for (i = 0; i < t; i++) { buff_tx[4 + i] = (uint8_t) files_name[file_mark][i]; }
            break;
        case PermitPro:
            buff_tx[0] = 0x02;
            if (temp_8b) {
                buff_tx[1] = 0x01;
                buff_tx[2] = port_no_tcp_pro;
            } else {
                buff_tx[1] = 0;
                buff_tx[2] = 0;
            }
            break;
        case Ack:
            buff_tx[0] = 0x03;
            buff_tx[1] = temp_8b;
            break;
    }
    //send the buffer to the socket
    write_to_socket = send(sock, buff_tx, buffer_size, 0);
    printf("wrote %d\n", write_to_socket);
    if (write_to_socket == -1) {//todo fixme!
        errnum = errno;
        fprintf(stderr, "Value of errno: %d\n", errno);
        perror("Error printed by perror");
    }
    if (write_to_socket > 0) { return 1; }//success
    else { return 0; }//fail
}

int verify(int socket, int mssg_expected, uint8_t buff[buffer_size]) {
    int err = 0;//1=not in format; 2= out of range
    uint16_t temp_16b;
    switch (mssg_expected) {
        case Hello:
            if ((buff[0] != 0x00) || (buff[1] != 0x00) || (buff[2] != 0x00)) { err = 1; };
            break;
        case AskFilm:
            if ((buff[0] != 0x01) || ((buff[1] == 0x00) && (buff[2] != 0x00))) { err = 1; };
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
            sendInvalid(socket, "Err. in verification of a message.");
            return 0;
        case 2:
            printf("Err. in verification of a message - out of range values.\n");
            sendInvalid(socket, "Err. in range of values.");
            return 0;
    }
}

void *pro_movie_streamer_thread_function(void *args) {
    struct premium_args *pro_args = (struct premium_args *) args;
    char *endptr;
    char line[256];
    int rate, r;
    int place = pro_args->movie_id;
    ssize_t write_to_socket;//read from socket
    //get local vars: mutex get client data
    if (mutex_clients) { while (mutex_clients) { ; }; }
    mutex_clients = 1;//mutex for touching the clients //fixme
    int self_row = movies_dim[place][0];
    int self_col = movies_dim[place][1];
    int self_fd = movies_fd[place];
    int self_sock = movie_sockets[place];
    char *self_name_file = files_name[place];
    struct sockaddr_in self_sock_struct = movie_sock_struct[place];
    mutex_clients = 0;
    char buf[buffer_size];

    printf("self_id = %d \n", self_fd);

    while (!quit) {
        //read 1 line
        if (fgets(line, sizeof(line), (FILE *) self_fd)) {
            //read row lines from file
            line[strlen(line) - 1] = 0;//delete end
            rate = strtol(line, &endptr, 10);//set rate
            rate = rate * (0.01 * (pro_args->speedup) + 1);
            //read file.txt into buffer
            r = read(self_fd, buf, self_row * self_col);
            //write to socket
            write_to_socket = send(pro_args->socket_info, buf, buffer_size, 0);

            //timer - sleep
            timer(rate);

        } else {
            close(self_fd);
            self_fd = open(files_name[place], O_RDONLY);
            if (self_fd == -1) {
                printf("ERR- can not open %s file\n", files_name[place]);
                return NULL;
            }//fixme
        }
    }
    return NULL;
}

void activate_response_pro(int pro_id) {
    printf("in activate_response_pro\n ");//todo del
    int pro_client_socket, trans_client_socket, b, l, c;
    struct sockaddr_in welcome_sock_struct_pro, client_sock_struct;//creat sockaddr struct:
    int read_from_socket;
    uint8_t buff[buffer_size];
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
    welcome_sock_struct_pro.sin_addr.s_addr = inet_addr("192.10.1.1");//fixme
    //bind socket
    b = bind(pro_client_socket, (struct sockaddr *) &welcome_sock_struct_pro, sizeof(welcome_sock_struct_pro));
    //todo b check for err
    //wait for a connection - listen
    l = listen(pro_client_socket, SOMAXCONN);
    if (!l) {//listening...
        printf("Listening PRO...\n");
        //STOPS UNTIL A NEW CLIENT -->> accept connection from an incoming client
        trans_client_socket = accept(pro_client_socket, (struct sockaddr *) &client_sock_struct, (socklen_t *) &c);
        //todo add check err
    } else { quit_pro = 1; }

    //-------------------------------------------------------------------
    //--Open a new thread: Transmitting movies
    struct premium_args *args;
    args->client_id = pro_id;
    args->movie_id = 0;
    args->socket_info = trans_client_socket;
    args->speedup = 0; //default
    pro_thread_quit_flag[pro_id] = 0;
    pthread_create(&thread_pro_movie_streamer, NULL, pro_movie_streamer_thread_function, (void *) args);

    //-------->> Infinite loop:
    while (!quit_pro) {
        read_from_socket = recv(pro_client_socket, buff, buffer_size, 0); //read UDP socket data into buffer
        if (buff[0] == 0x01) {//change film
            if (verify(pro_client_socket, AskFilm, buff)) {
                uint16_t station_r_b = ntohs(*(uint16_t *) &buff[1]);
                station_req = (int) (station_r_b);
                new_req = 1;
                send_ctrl(pro_client_socket, Announce, 0, station_req, 0, 0);
                //stop thread
                pro_thread_quit_flag[pro_id] = 1;
                pthread_join(thread_pro_movie_streamer, NULL);//block until thread is finished - stop bugs
                //open new thead
                //--Open a new thread: Transmitting movies
                struct premium_args *args;
                args->movie_id = station_req;
                pro_thread_quit_flag[pro_id] = 0;
                pthread_create(&thread_pro_movie_streamer, NULL, pro_movie_streamer_thread_function, (void *) args);
            }
        }
        if (buff[0] == 0x02) { sendInvalid(pro_client_socket, "Already in pro ERR."); }
        if (buff[0] == 0x03) {
            send_ctrl(pro_client_socket, Ack, 0, 0x03, 0, 0);
            //stop thread
            pro_thread_quit_flag[pro_id] = 1;
            pthread_join(thread_pro_movie_streamer, NULL);//block until thread is finished - stop bugs
            //open new thead
            //--Open a new thread: Transmitting movies
            struct premium_args *args;
            args->speedup = (int) buff[1];
            pro_thread_quit_flag[pro_id] = 0;
            pthread_create(&thread_pro_movie_streamer, NULL, pro_movie_streamer_thread_function, (void *) args);
        }
        if (buff[0] == 0x04) {
            send_ctrl(pro_client_socket, Ack, 0, 0x03, 0, 0);
            close(pro_client_socket);
            pro_client_tracker[pro_id] = 0;
            quit_pro = 1;
            //stop thread
            pro_thread_quit_flag[pro_id] = 1;
            pthread_join(thread_pro_movie_streamer, NULL);//block until thread is finished - stop bugs
        }
    }//end while ! quit pro

}//end react pro

void activate_response(int sock) {
    int read_from_socket;
    uint8_t buff[buffer_size];
    read_from_socket = recv(sock, buff, buffer_size, 0); //read UDP socket data into buffer
    if (buff[0] == 0x01) {
        if (verify(sock, AskFilm, buff)) {
            uint16_t station_r_b = ntohs(*(uint16_t *) &buff[1]);
            int station_r = (int) (station_r_b);
            send_ctrl(sock, Announce, 0, station_r, 0, 0);
        }
    }
    if (buff[0] == 0x02) {
        if (verify(sock, GoPro, buff)) {
            int id_pro = check_pro_permit();
            if (id_pro) {
                send_ctrl(sock, GoPro, 0, 0x01, 0, 0);
                activate_response_pro(id_pro);//block until listen to new client
            } else { send_ctrl(sock, GoPro, 0, 0, 0, 0); }
        }
    }
    if (read_from_socket <= 0) { client_tracker[mark] = -2; }
}



//--MAIN thread functions

void *send_film_thread_function(void *arg) {
//    int *a = (int *) arg;
//    printf("%d\n", *a);
//    int i = (int) &a;
    printf("send_film_thread_function\n");
//    int i = (int) &arg;
    int i;
    if (touch_pos == 0) {
        touch_pos = 1;
        i = pos;
        pos = pos + 1;
        touch_pos = 0;
    }
    else {
        printf("Unable to touch\n");
        i = 0;
    }
    char c[1];
    int r;
    char *endptr;
    char line[256];
    char buf_rate[2];
    int header = ON;
    int rate;
    ssize_t write_to_socket;//read from socket
    //get local vars: mutex get client data
    if (mutex_clients) { while (mutex_clients) { ; }; }
    mutex_clients = 1;//mutex for touching the clients //fixme
    int self_row = movies_dim[i][0];
    int self_col = movies_dim[i][1];
    int self_fd = movies_fd[i];
    printf("here 4\n");
    int self_sock = movie_sockets[i];
    char *self_name_file = files_name[i];
    printf("self_row %d self_col %d self_fd %d self_sock %d\n", movies_dim[i][0], movies_dim[i][1], movies_fd[i],
           movie_sockets[i]);
    struct sockaddr_in self_sock_struct = movie_sock_struct[i];
    mutex_clients = 0;
    char buf[buffer_size];

    printf("self_id = %d \n", self_fd);

    while (!quit) {
        //read 1 line
        if (read(self_fd, buf_rate, 2)!=-1) {
            if(buf_rate[1]=='\n'){
                //read row lines from file
                buf_rate[1] = 0;//delete end
                rate = strtol(buf_rate, &endptr, 10);//set rate
            }
            else{
                rate = strtol(buf_rate, &endptr, 10);//set rate
                read(self_fd, buf_rate, 1);
            }
            //read file.txt into buffer
            r = read(self_fd, buf, self_row * self_col);
            //write to socket
            write_to_socket = sendto(self_sock, buf, buffer_size, 0, (struct sockaddr *) &self_sock_struct,
                                     sizeof(self_sock_struct));
            //timer - sleep
            timer(rate);
        } else {
            close(self_fd);
            self_fd = open(files_name[i], O_RDONLY);
            if (self_fd == -1) {
                printf("ERR- can not open %s file\n", files_name[i]);
                return NULL;
            }//fixme
        }
    }
    printf("quit in send_film_thread_function\n");
    return NULL;
}

//movie_streamer_thread_function
void *movie_streamer_thread_function(void *v) {
    //variables declarations
    int myUDPsocket, i;//temp keeper to fill in the array movie_sockets
    u_char ttl;//temp to set properties to M.C.
    ttl = 32;
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
        printf("sock_struct.sin_addr.s_addr = mc_addr = %x\n", mc_addr + (uint32_t) i);//todo del
        //multicast definitions
        setsockopt(myUDPsocket, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));
        //append to the array movie_sockets
        //todo add err case
        movie_sockets[i] = myUDPsocket;
        movie_sock_struct[i] = sock_struct;
        //create a transmission thread for this movie
        udp_socket_thread_ids[no_files] = i;
        pthread_create(&movie_threads[i], NULL, send_film_thread_function, (void *) (udp_socket_thread_ids + i));
        printf("inter thread = send_film_thread_function =  %d\n", i);
    }
    //hold untill threads are dead
    for (i = 0; i < no_files; i++) { pthread_join(movie_threads[i], NULL); }
    return NULL;
}

void *active_client_thread_function(void *v) {
    printf("-----------in active_client_thread_function");
    //variables
    int read_from_socket, t;
    uint8_t buff_rx[buffer_size];
    //mutex get client data
    if (mutex_clients) { while (mutex_clients) { ; }; }
    mutex_clients = 1;//mutex for touching the clients //fixme
    int self_mark = mark;
    int self_id = client_tracker[mark];
    mutex_clients = 0;
    printf("self_id = %d \n", self_id);
    //receive hello
    if (!time_out(self_id)) { read_from_socket = recv(self_id, buff_rx, buffer_size, 0); }
    if (read_from_socket <= 0) { printf("Err in 'recv' function (Hello).\n"); }// err massage
    //write to socket Welcome
    if (verify(self_id, Hello, buff_rx)) { t = send_ctrl(self_id, Welcome, 0, 0, 0, 0); }
    if (!t) { printf("Err in 'send' function (Wlcome).\n"); }
    //proper responses for requests: get film, change channel, pro, speed up, release...
    while (client_tracker[mark] != -2) { activate_response(self_id); }
}

void *listen_to_clients_thread_function(void *v) {
    printf("listen_to_clients_thread_function\n");
    //thread reletad variables declarations
    pthread_t thread_id_new_client;
    int flag;//flags that a new cell has been fount in the new clients array
    //socket related variables declarations
    int b, l, c, i;//temp holders
    int welcomeTCPsocket;
    int newClient_TCPsocket;
    struct sockaddr_in welcome_sock_struct;//creat sockaddr struct:
    struct sockaddr_in client_sock_struct;//socket for client
    int myIP;//todo check!
    //assign fix vars
    c = sizeof(struct sockaddr_in);
    //fixme myIP = get_IP();//get my IP address as string

    //--Open a Welcome socket (TCP control with clients)
    /*SOCKET PROCESS*/
    //open a TCP socket
    welcomeTCPsocket = socket(AF_INET, SOCK_STREAM, 0);
    //set vars to sock struct - srv:
    welcome_sock_struct.sin_family = AF_INET;
    welcome_sock_struct.sin_port = htons(port_no_tcp_ctrl);
    welcome_sock_struct.sin_addr.s_addr = inet_addr("192.10.5.1");//fixme
    //bind socket
    b = bind(welcomeTCPsocket, (struct sockaddr *) &welcome_sock_struct, sizeof(welcome_sock_struct));
    //todo b check for err

    //-------->> Infinite loop:
    while (!quit) {
        //wait for a connection - listen
        l = listen(welcomeTCPsocket, SOMAXCONN);
        if (!l) {//listening...
            printf("Listening...\n");
            //STOPS UNTIL A NEW CLIENT -->> accept connection from an incoming client
            newClient_TCPsocket = accept(welcomeTCPsocket, (struct sockaddr *) &client_sock_struct, (socklen_t *) &c);
            //todo add check err
            printf("A new client has joined AsciiFlix! Yey!\n");
            //attach new client to a new thread
            if (mutex_clients) { while (mutex_clients) { ; }; }
            mutex_clients = 1;//mutex for touching the clients //fixme
            flag = 0;
            for (i = 0; i < client_max_size; i++) {
                if ((client_tracker[i] == -3) && (!flag)) {
                    client_tracker[i] = newClient_TCPsocket;
                    printf("newClient_TCPsocket = %d \n", newClient_TCPsocket);
                    mark = i;
                    flag = 1;
                }//todo check if we need to pass more arguments to the next thread--client_sock_struct??
            }
            mutex_clients = 0;
            //--Open a new thread: active_client_thread
            pthread_create(&thread_id_new_client, NULL, active_client_thread_function, NULL);
        } else { printf("ERR listening\n"); } //todo err
    }//while ! quit
}

////~////~//// MAIN ////~////~////
int main(int argc, char **argv) {
    int i;
    //~ First activation:
    for (i = 0; i < client_max_size; i++) { client_tracker[i] = -3; }
    //--declare main variables
    char input_char; //input for 'quit' from user
    //Decode to variables input from command line
    port_no_tcp_ctrl = atoi(argv[1]); //port number of the welcome TCP
    port_no_tcp_pro = atoi(argv[2]); //port number of the pro TCP
    port_no_udp = atoi(argv[4]); //port number of the broadcasting the movies
    mc_addr = inet_addr(argv[3]); //multicast addr
    //Files input
    no_files = argc - 5; //number of files to screen
    for (i = 0; i < no_files; i++) { files_name[i] = argv[5 + i]; } //input files names in name array
    //Threads variables
    pthread_t thread_id_movie_streamer, thread_id_welcome_socket; //create thread variable
    //prep work for thread- open files to read
    int fd, r;
    char buf[5];
    for (i = 0; i < no_files; i++) {
        //printf("%s\n", files_name[i]);
        fd = open(files_name[i], O_RDONLY);
        if (fd == -1) {
            printf("ERR- can not open %s file\n", files_name[i]);
            //return -1;
        }//fixme
        movies_fd[i] = fd;
        r = read(fd, buf, 5);
        movies_dim[i][0] = ((int) (buf[0] - '0')) * 10 + (int) (buf[1] - '0');
        movies_dim[i][1] = ((int) (buf[3] - '0')) * 10 + (int) (buf[4] - '0');
        printf("movies_dim row %d, col %d :\n", movies_dim[i][0], movies_dim[i][1]);
    }

    //User inform:
    printf("\nHello user and welcome to AsciiFlix SERVER side.\n");
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    printf("Technical information:\nTCP control port number: %d\n", port_no_tcp_ctrl);
    printf("UDP movie streamer port number: %d\nUDP movie streamer multicast address: %s\n", port_no_udp, argv[3]);
    printf("TCP pro user port number: %d\n", port_no_tcp_pro);
    printf("Today movies are: ");
    for (i = 0; i < no_files; i++) { printf("%s ", files_name[i]); }

    //-------------------------------------------------------------------
    //--Open a new thread: Transmitting movies
    pthread_create(&thread_id_movie_streamer, NULL, movie_streamer_thread_function, NULL);

    //-------------------------------------------------------------------
    //--Open a new thread: listening for new clients
    pthread_create(&thread_id_welcome_socket, NULL, listen_to_clients_thread_function, NULL);


    while (!quit) {
        //quit option - input from user
        printf("\nPress q + Enter to quit the program at any time\n");
        scanf("%c", &input_char);
        fflush(stdin);
        // handle input fixme!!!
        if (input_char == 'q') { quit = 1; }    // check input
        else {
            printf("\n");
            scanf("%c", &input_char);
        }
    }

    //-------------------------------------------------------------------
    pthread_join(thread_id_movie_streamer, NULL);//block until thread is finished - stop bugs
    pthread_join(thread_id_welcome_socket, NULL);//block until thread is finished - stop bugs
    return 0;//just in case
}//end main
// @ Ilan & Shir.