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
int files_fd[movies_max_size]; //this array keeps the fd int number of the movies txt files
int movies_bit_flag[movies_max_size]; //this array is to keep track of movies that need to switch frames
int no_files;

//--Help functions
//transmit new frame in Transmitting movies section
void *movie_streamer_thread_function(void *vargp)
{
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
    printf("new_client_thread_function\n");
    //welcome socket already open todo notice

    //todo do while?

    //listening

    //new client - open a new thread

}

////~////~//// MAIN ////~////~////
int main(int argc, char** argv) {
    //~ First activation:

    //--declare main variables
    //Decode to variables input from command line
    int port_no_tcp_ctrl = atoi(argv[1]); //port number of the welcome TCP
    int port_no_tcp_pro = atoi(argv[2]); //port number of the pro TCP
    int port_no_udp = atoi(argv[4]); //port number of the broadcasting the movies
    in_addr_t mc_addr = inet_addr(argv[3]); //multicast addr
    //Files input
    no_files = argc-5; //number of files to screen
    for(int i=0; i<no_files;i++){files_name[i] = argv[5+i];} //input files names in name array
    //Threads variables
    pthread_t thread_id_movie_streamer, thread_id_new_client; //create thread variable todo check second purpose
    //User inform:
    printf("Today movies are: ");
    for(int i=0;i<no_files;i++){printf("%s ", files_name[i]);}
    printf("\nStart streaming movies to the multicast group: %s\n", argv[3]);

    //--Open a Welcome socket (TCP control with clients)

    //-------------------------------------------------------------------
    //prep work for thread- open files to read
    int fd;
    for(int i=0; i<no_files;i++){
        //printf("%s\n", files_name[i]);
        fd = open( files_name[i], O_RDONLY );
        //todo if(fd==-1){ printf("ERR- can not open %s file\n", files_name[i] ); return -1; }
    }
    //--Open a new thread: Transsmiting movies
    pthread_create(&thread_id_movie_streamer, NULL, movie_streamer_thread_function, NULL);


    //~ Infinite activation: while (1)


    //-------------------------------------------------------------------
    //--Open a new thread: listening for new clients
    pthread_create(&thread_id_new_client, NULL, new_client_thread_function, NULL);
    //User inform
    printf("Ready for clients,\nlistening...");

    //receive hello
    //--if a new client came -> open a new child process for client communication
    //send welcome (hello response)
    //establish a connection ++ Timeout
    //proper responses for requests: get film, change channel, pro, speed up, release...

    //--if a new client came -> father process go back to listen

    pthread_join(thread_id_movie_streamer, NULL);//block until thread is finished - stop bugs
    exit(0);//just in case
}//end main
// @ Ilan & Shir.