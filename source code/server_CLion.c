/*====================================================================================
 Final project in Lab session of CCS2 course.
 Written by Ilan Klein & Shir Granit.


				----------  Ascii-Film App:  Server side  ----------


	compile with gcc server.c -o film_server
	run with ./film_server <tcp control port> <tcp pro port> <multicast ip adr>
							<udp port> <file1> <file2> <file3> <file4> ...
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



//--Defines:
#define client_max_size 50
#define client_pro_size 2
#define movies_max_size 10
#define buffer_size 1024

//--Global variables
/* Array of clients tracker = size clients_max_size
 * 0 = no client ,default at restart
 * 1 = client has tried to establish a new connection
 * 2 = client is connected and online
 * -1 = client ERR
 * -2 = no client - mark that this cell is not the active ending of the clients list
*/
int client_tracker[client_max_size] = {0};
int pro_client_tracker[client_pro_size] = {0};
int mutex_clients = 0; //mutex lock access to the arrays: 0 = available, 1 = taken by another thread
//Movies
char* files_name[movies_max_size]; //this array is to keep track on the movies we screen
int movies_fd[movies_max_size]; //this array keeps the fd int number of the movies txt files
int movies_bit_flag[movies_max_size]; //this array is to keep track of movies that need to switch frames
int no_files;//mark the number of files an input part in main
//Input from user
int port_no_tcp_ctrl; //port number of the welcome TCP
int port_no_tcp_pro; //port number of the pro TCP
int port_no_udp; //port number of the broadcasting the movies
in_addr_t mc_addr;//multicast addr

//--Help functions
//transmit new frame in Transmitting movies section
void *movie_streamer_thread_function(void *v)
{
    printf("----------movie_streamer_thread_function\n");//todo
    //variables declarations
    int movie_sockets[no_files];//keep UDPs fd numbers
    int myUDPsocket;//temp keeper to fill in the array movie_sockets
    u_char ttl;//temp to set properties to M.C.
    ttl = 32;
    struct sockaddr_in sock_struct;//creat sockaddr struct:
    struct ip_mreq mreq;//create multicast request

    /* UDP SOCKET PROCESS */
    for(int i=0;i<no_files;i++){
        //open a UDP socket
        myUDPsocket = socket(AF_INET , SOCK_DGRAM , 0);
        //set socket properties
        sock_struct.sin_family = AF_INET;
        sock_struct.sin_port = htons(port_no_udp);
        sock_struct.sin_addr.s_addr = mc_addr;
        //multicast definitions
        setsockopt( myUDPsocket , IPPROTO_IP , IP_MULTICAST_TTL ,&ttl , sizeof(ttl));
        //append to the array movie_sockets
        //todo add err
        movie_sockets[i] = myUDPsocket;
    }


    // Infinite loop:
    while(0)
    {
        int i=0;
    }


    //todo stream:
    //while 1
//    printf("in movie streamer\n");
//    for(int i=0;i<no_files;i++){
//        printf("%s\n",files_name[i]);
//    }
    //use the select function to know which movie to switch frame
    return NULL;
}

void *new_client_thread_function(void *v){
    printf("----------new_client_thread_function\n");

    //--Open a Welcome socket (TCP control with clients)
    //welcome socket already open todo notice

    //todo while 1
    //while(1)
    // Infinite loop:
    while(0)
    {
        int i=0;
    }

    //listening

    //new client - open a new thread
    //--if a new client came -> open a new child process for client communication


    //--if a new client came -> father process go back to listen

}

void *active_client_thread_function(void *v){
    printf("-----------in active_client_thread_function");
    //receive hello
    //send welcome (hello response)
    //establish a connection ++ Timeout
    //proper responses for requests: get film, change channel, pro, speed up, release...

}

////~////~//// MAIN ////~////~////
int main(int argc, char** argv) {
    //~ First activation:

    //--declare main variables
    //Decode to variables input from command line
    port_no_tcp_ctrl = atoi(argv[1]); //port number of the welcome TCP
    port_no_tcp_pro = atoi(argv[2]); //port number of the pro TCP
    port_no_udp = atoi(argv[4]); //port number of the broadcasting the movies
    mc_addr = inet_addr(argv[3]); //multicast addr
    //Files input
    no_files = argc-5; //number of files to screen
    for(int i=0; i<no_files;i++){files_name[i] = argv[5+i];} //input files names in name array
    //Threads variables
    pthread_t thread_id_movie_streamer, thread_id_new_client; //create thread variable todo check second purpose
    //prep work for thread- open files to read
    int fd;
    for(int i=0; i<no_files;i++){
        //printf("%s\n", files_name[i]);
        fd = open( files_name[i], O_RDONLY );
        //fixme if(fd==-1){ printf("ERR- can not open %s file\n", files_name[i] ); return -1; }
    }

    //User inform:
    printf("\nHello user and welcome to AsciiFlix SERVER side.\n");
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    printf("Technical information:\nTCP control port number: %d\n",port_no_tcp_ctrl);
    printf("UDP movie streamer port number: %d\nUDP movie streamer multicast address: %s\n", port_no_udp, argv[3]);
    printf("TCP pro user port number: %d\n",port_no_tcp_pro);

    //-------------------------------------------------------------------
    //--Open a new thread: Transmitting movies
    pthread_create(&thread_id_movie_streamer, NULL, movie_streamer_thread_function, NULL);

    //User inform:
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    printf("Today movies are: ");
    for(int i=0;i<no_files;i++){printf("%s ", files_name[i]);}
    printf("\nStart streaming movies to the multicast group: %s\n", argv[3]);

    //-------------------------------------------------------------------
    //--Open a new thread: listening for new clients
    pthread_create(&thread_id_new_client, NULL, new_client_thread_function, NULL);

    //User inform
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    printf("Ready for clients! listening...\n");

    //-------------------------------------------------------------------
    pthread_join(thread_id_movie_streamer, NULL);//block until thread is finished - stop bugs
    pthread_join(thread_id_new_client, NULL);//block until thread is finished - stop bugs
    exit(0);//just in case
}//end main
// @ Ilan & Shir.