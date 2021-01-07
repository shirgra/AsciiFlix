///*====================================================================================
// Final project in Lab session of CCS2 course.
// Written by Ilan Klein & Shir Granit.
//
//
//				----------  Ascii-Film App:  Server side  ----------
//
//
//	compile with gcc server.c -o film_server
//	run with ./film_server <tcp control port> <tcp pro port> <multicast ip adr>
//							<udp port> <file1> <file2> <file3> <file4> ...
//  ==================================================================================*/
//
////--Includes:
//#include <stdio.h>
//#include <stdlib.h>
//#include <unistd.h> //for sleep()
//#include <pthread.h> //for threads
//#include <sys/types.h>
//#include <sys/socket.h>
//#include <arpa/inet.h>
//#include <string.h>
//#include <fcntl.h> //read files
//#include <ifaddrs.h>
//
////--Defines:
//#define client_max_size 50
//#define client_pro_size 2
//#define movies_max_size 10
//#define buffer_size 1024
//
////--Global variables
//int quit=0;//for when the user wants to quit
///* Array of clients tracker = size clients_max_size
// * -3 = no client ,default at restart
// * -2 = no client - mark that this cell is not the active ending of the clients list
// * any positive int = client is connected and online*/
//int client_tracker[client_max_size];
//int pro_client_tracker[client_pro_size] = {0};
//int mutex_clients = 0; //mutex lock access to the arrays: 0 = available, 1 = taken by another thread
////Movies
//char* files_name[movies_max_size]; //this array is to keep track on the movies we screen
//int movies_fd[movies_max_size]; //this array keeps the fd int number of the movies txt files
//int movies_bit_flag[movies_max_size]; //this array is to keep track of movies that need to switch frames
//int no_files;//mark the number of files an input part in main
////Input from user
//int port_no_tcp_ctrl; //port number of the welcome TCP
//int port_no_tcp_pro; //port number of the pro TCP
//int port_no_udp; //port number of the broadcasting the movies
//in_addr_t mc_addr;//multicast addr
//
////--Side Help functions
////fixme! get self id
////int get_IP(void){//return the user IP address
////    struct ifaddrs *id;
////    int val;
////    val = getifaddrs(&id);
////    printf("Network Interface Name :- %s\n",id->ifa_name);
////    printf("Network Address of %s :- %d\n",id->ifa_name,id->ifa_addr);
////    printf("Network Data :- %d \n",id->ifa_data);
////    printf("Socket Data : -%c\n",id->ifa_addr->sa_data);
////    return id->ifa_addr;
////}
//
//
////--MAIN Help functions
////transmit new frame in Transmitting movies section
//void *movie_streamer_thread_function(void *v)
//{
//    //variables declarations
//    int movie_sockets[no_files];//keep UDPs fd numbers
//    int myUDPsocket;//temp keeper to fill in the array movie_sockets
//    u_char ttl;//temp to set properties to M.C.
//    ttl = 32;
//    struct sockaddr_in sock_struct;//creat sockaddr struct:
//    struct ip_mreq mreq;//create multicast request
//
//    /* UDP SOCKET PROCESS */
//    for(int i=0;i<no_files;i++){ //to every movie:
//        //open a UDP socket
//        myUDPsocket = socket(AF_INET , SOCK_DGRAM , 0);
//        //set socket properties
//        sock_struct.sin_family = AF_INET;
//        sock_struct.sin_port = htons(port_no_udp);
//        sock_struct.sin_addr.s_addr = mc_addr;
//        //multicast definitions
//        setsockopt( myUDPsocket , IPPROTO_IP , IP_MULTICAST_TTL ,&ttl , sizeof(ttl));
//        //append to the array movie_sockets
//        //todo add err case
//        movie_sockets[i] = myUDPsocket;
//    }
//
//    // Infinite loop:
//    while(!quit)
//    {
//        int i=0;
//    }
//
//    //todo stream:
//    //use the select function to know which movie to switch frame
//
//    //send a massage:
//    //write to socket
//    //int write_to_socket; char buf[buffer_size];//buffer for data;
//    //write_to_socket = sendto( myUDPsocket , buf , buffer_size , 0 , (struct sockaddr *) &sock_struct , sizeof(sock_struct)) ;
//
//    return NULL;
//}
//
//void *active_client_thread_function(void *v){
//    printf("-----------in active_client_thread_function");
//    //write to socket
//    // write_to_socket = send( a , buf , buffer_size , 0 );
//
//    //receive hello
//    //send welcome (hello response)
//    //establish a connection ++ Timeout
//    //proper responses for requests: get film, change channel, pro, speed up, release...
//
//}
//
//void *listen_to_clients_thread_function(void *v){
//    //thread reletad variables declarations
//    pthread_t thread_id_new_client;
//    int flag;//flags that a new cell has been fount in the new clients array
//    //socket related variables declarations
//    int b,l,c,i;//temp holders
//    int welcomeTCPsocket;
//    int newClient_TCPsocket;
//    struct sockaddr_in welcome_sock_struct;//creat sockaddr struct:
//    struct sockaddr_in client_sock_struct;//socket for client
//    int myIP;//todo check!
//    //assign fix vars
//    c = sizeof(struct sockaddr_in);
//    //fixme myIP = get_IP();//get my IP address as string
//
//    //--Open a Welcome socket (TCP control with clients)
//    /*SOCKET PROCESS*/
//    //open a TCP socket
//    welcomeTCPsocket = socket(AF_INET , SOCK_STREAM , 0);
//    //set vars to sock struct - srv:
//    welcome_sock_struct.sin_family = AF_INET ;
//    welcome_sock_struct.sin_port = htons(port_no_tcp_ctrl) ;
//    welcome_sock_struct.sin_addr.s_addr = inet_addr("192.10.1.1");//fixme
//    //bind socket
//    b = bind( welcomeTCPsocket , (struct sockaddr *) &welcome_sock_struct, sizeof(welcome_sock_struct));
//    //todo b check for err
//
//    //User inform
//    //printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n"); todo
//    //printf("Ready for clients!\n");
//
//    //-------->> Infinite loop:
//    while(!quit){
//        //wait for a connection - listen
//        l = listen(welcomeTCPsocket, SOMAXCONN);
//        if(!l){//listening...
//            printf("Listening...\n");
//            //STOPS UNTIL A NEW CLIENT -->> accept connection from an incoming client
//            newClient_TCPsocket = accept(welcomeTCPsocket, (struct sockaddr *)&client_sock_struct, (socklen_t*)&c);
//            //todo add check err
//            printf("A new client has joined AsciiFlix! Yey!\n");
//            //attach new client to a new thread
//            while(!mutex_clients){mutex_clients=1;}//mutex for touching the clients //fixme
//            flag=0;
//            for(i=0;i<client_max_size;i++){
//                if((client_tracker[i]==-3)&&(!flag)){
//                    client_tracker[i]=newClient_TCPsocket;
//                    flag=1;
//                }//todo check if we need to pass more arguments to the next thread--client_sock_struct??
//            }
//            mutex_clients=0;
//            //--Open a new thread: active_client_thread
//            pthread_create(&thread_id_new_client, NULL, active_client_thread_function, NULL);
//        }
//        else{ printf("ERR\n");} //todo err
//    }
//}
//
//////~////~//// MAIN ////~////~////
//int main(int argc, char** argv) {
//    //~ First activation:
//    for(int i=0; i<client_max_size; i++){client_tracker[i]= -3;}
//
//    //--declare main variables
//    char input_char; //input for 'quit' from user
//    //Decode to variables input from command line
//    port_no_tcp_ctrl = atoi(argv[1]); //port number of the welcome TCP
//    port_no_tcp_pro = atoi(argv[2]); //port number of the pro TCP
//    port_no_udp = atoi(argv[4]); //port number of the broadcasting the movies
//    mc_addr = inet_addr(argv[3]); //multicast addr
//    //Files input
//    no_files = argc-5; //number of files to screen
//    for(int i=0; i<no_files;i++){files_name[i] = argv[5+i];} //input files names in name array
//    //Threads variables
//    pthread_t thread_id_movie_streamer, thread_id_welcome_socket; //create thread variable
//    //prep work for thread- open files to read
//    int fd;
//    for(int i=0; i<no_files;i++){
//        //printf("%s\n", files_name[i]);
//        fd = open( files_name[i], O_RDONLY );
//        //fixme if(fd==-1){ printf("ERR- can not open %s file\n", files_name[i] ); return -1; }
//    }
//
//    //User inform:
//    printf("\nHello user and welcome to AsciiFlix SERVER side.\n");
//    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
//    printf("Technical information:\nTCP control port number: %d\n",port_no_tcp_ctrl);
//    printf("UDP movie streamer port number: %d\nUDP movie streamer multicast address: %s\n", port_no_udp, argv[3]);
//    printf("TCP pro user port number: %d\n",port_no_tcp_pro);
//    printf("Today movies are: ");
//    for(int i=0;i<no_files;i++){printf("%s ", files_name[i]);}
//
//    //quit option - input from user
//    printf("\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
//    printf("\nPress q + Enter to quit the program at any time\n");
//    scanf("%c",&input_char);
//
//    //-------------------------------------------------------------------
//    //--Open a new thread: Transmitting movies
//    pthread_create(&thread_id_movie_streamer, NULL, movie_streamer_thread_function, NULL);
//
//    //-------------------------------------------------------------------
//    //--Open a new thread: listening for new clients
//    pthread_create(&thread_id_welcome_socket, NULL, listen_to_clients_thread_function, NULL);
//
//    // handle input fixme!!!
//    if(input_char=='q'){quit=1;}    // check input
//    else{
//        printf("Your input was incorrect. Press q + Enter to quit. \n");
//        scanf("%c",&input_char);
//    }
//
//    //-------------------------------------------------------------------
//    pthread_join(thread_id_movie_streamer, NULL);//block until thread is finished - stop bugs
//    pthread_join(thread_id_welcome_socket, NULL);//block until thread is finished - stop bugs
//    return 0;//just in case
//}//end main
//// @ Ilan & Shir.