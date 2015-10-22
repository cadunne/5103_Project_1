/**********************************/
/****** CSCI 5103 Project 1 *******/
/*** OS Support for Concurrency ***/
/**********************************/
/**** Connor Dunne // dunne065 ****/
/***** Jane Kagan // kagan009 *****/
/*** Karel Kalthoff // kalt0032 ***/
/**********************************/

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <assert.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <dirent.h>
#include <sys/time.h>

int aio_read;
int port;
int sockfd;

typedef struct {
   	short		sin_family;
    	unsigned short	sin_port;
	struct in_addr	sin_addr;
   	char		sin_zero[8];
} sockaddr_in;

typedef struct {
	int clientfd;
	int threadnum;
	struct timeval timer;
} threadarg;

struct sockaddr_in server;

struct timeval start;
float total;

// Function that the threads execute
void *handle_request(void *arg) {
	
	// 1KB Buffer to read into 
	char buffer[1024];
	int readbytes; 
	threadarg myarg;
	myarg = *((threadarg*) arg);
	int clientfd = myarg.clientfd;
	int i = myarg.threadnum;
	
	// Timing per thread
	struct timeval endtime;
	struct timeval starttime = myarg.timer;

  	// Initial read
	if ((readbytes = read(clientfd, buffer, sizeof(buffer))) < 0) {
		fprintf(stderr, "Read from client failed.\n");   	
	}
	
  	// If there was something left to read, go back for more until none is left
	while (readbytes > 0) {
		if ((readbytes = read(clientfd, buffer, sizeof(buffer))) < 0) {
			fprintf(stderr, "Read from client failed.\n");
		}	
	}
 
  	// Close socket
	close(clientfd);

	// Stop timer
	gettimeofday(&endtime, NULL);

	fprintf(stderr, "Request completed by thread %d.\n Elapsed time: %d microseconds.\nClient connection closed.\n\n", i, (endtime.tv_usec - starttime.tv_usec));


  	// Exit thread
	pthread_exit(0);
}

int thread_impl() {
    
   	int clientsockfd;
   	struct sockaddr_in client;
	int threadno = 0;

    	fprintf(stderr, "This server will use threads to service each new connection.\n");
    
   	// Create sockaddr object for the server
    	server.sin_family = AF_INET;
    	server.sin_addr.s_addr = INADDR_ANY;
    	server.sin_port = htons(port);
    	sockfd = socket(AF_INET, SOCK_STREAM, 0);

    	// Open socket
    	if (sockfd < 0) 
        	fprintf(stderr, "Error opening socket.\n");

    	// Bind socket to port
    	if (bind(sockfd, (struct sockaddr *)&server, sizeof(server)) < 0) {
        	fprintf(stderr, "Error binding socket.\n");
		exit(1);
    	}	

    	fprintf(stderr, "Opened socket and bound to port %d.\n", port);
    
    	// Listen on port
    	listen(sockfd, 5);
    
  	int client_len = sizeof(client);    

    	while (clientsockfd = accept(sockfd, (struct sockaddr *) &client, &client_len))
    	{
		// Create thread's timer
		struct timeval thread_timer;
		gettimeofday(&thread_timer, NULL);

        	// Create thread object
        	pthread_t client_thread;        

        	// Allocate thread arguments
		threadarg *thread_arg = malloc(sizeof(*thread_arg));
		thread_arg->clientfd = clientsockfd;
		thread_arg->threadnum = threadno;
		thread_arg->timer = thread_timer;        

        	// Create and detach threads
        	if (pthread_create(&client_thread, NULL, &handle_request, thread_arg) < 0) {
            		fprintf(stderr, "Failed to create thread\n");
       	 	}

        	if (pthread_detach(client_thread) < 0) {
            		fprintf(stderr, "Failed to detach thread.\n");
        	}

		threadno++;

    	}

	return 0;

}

int polling_impl() {
    fprintf(stderr, "Polling\n");
    if (aio_read) {


//         The server will handle all I/O operations (connections and receiving data) with a single thread using
// asynchronous I/O and polling. When a new connection is accepted, the server will store the socket
// descriptor (in a data-structure of your choice) and then issue an asynchronous read from the socket:
// a) Using aio_read which will return immediately. To see whether the read operation is done or
// not for each socket descriptor, the server will keep checking them one by one by polling.
// b) Using read but with control flags set to make it asynchronous (see fnctl) which will return
// immediately. This is the older Unix style


        //Listen for all clients (or, one at a time?)

        // 

        server.sin_family = AF_INET;
        server.sin_addr.s_addr = INADDR_ANY;
        server.sin_port = htons(port);
        sockfd = socket(AF_INET, SOCK_STREAM, 0);

        // Open socket
        if (sockfd < 0) 
            fprintf(stderr, "Error opening socket.\n");

        // Bind socket to port
        if (bind(sockfd, (struct sockaddr *)&server, sizeof(server)) < 0) {
            fprintf(stderr, "Error binding socket.\n");
        exit(1);
        }

        fprintf(stderr, "Opened socket and bound to port %d.\n");


        // while (clientsockfd = accept(sockfd, (struct sockaddr *) &client, &client_len))
        // {
        //    //create thread
        // }

        // while ((non-blocking)listen for clients to get connections)
        // {
        //     //IF new client, do all adding, setup, etc
        //         //change flags so it's non-blocking http://stackoverflow.com/questions/914463/in-c-how-to-make-a-file-descriptor-blocking
        //         //For Karel: look at F_NOTIFY
        //         //add to gFD_SET


        //     //non-blocking call to handleConnectionMethod (different for each implementation)
        // }


    }
    else {

    }
    return 0;
}

int handleConnectionMethod(){
    //Precondition: everything "set up" in FD_Set, stable state of connections
    //still pseudocode
    //MAKE GLOBAL FDSET called gFD_SET

    //QUESTION: Are we running multiple reads at once, or one at a time? e.g. select -> read, select -> read


    //CASE 1: bFD_SET is full of fd's, and we want to select 1 to run. No others are running
        //ASSUMPTION: fd is non-blocking
        //"select" fd to run from array, somehow (example: fd_set[0])
        //call "read" on "current_fd" (non-blocking)
        //somehow "mark" what fd is being read .. maybe have "states" within the fd_set ASK IN OFFICE HOURS

    //Case 2: bFD_SET has one fd is reading, but not finished


        //CHECK to see if currently reading anything
        //IF WE ONLY DO ONE READ AT ONCE:
            //Do nothing
        //Otherwise:
            //Look for next viable read, start 'reading' it.

    //Case 3: bFD_SET has one complete fd, done reading, and everything else is not start
        
        //stop timer or something here
        //Do all the cleanup from the FD
            //Especially cleanup the buffer!
            //Maybe print out the buffer .. or check it .... not sure

        //clean up the fd_set in some way?


//emphasis on maintaining the fd_set, making sure selection / deletion is done properly, etc.... could be a queue








// The server will handle all I/O operations (connections and receiving data) with a single thread using
// asynchronous I/O and polling. When a new connection is accepted, the server will store the socket
// descriptor (in a data-structure of your choice) and then issue an asynchronous read from the socket:

// b) Using read but with control flags set to make it asynchronous (see fnctl) which will return
// immediately. This is the older Unix style




    return 0;

}


//Connor - We can combine some of our code later on
int select_impl() {

    int clientsockfd;
    struct sockaddr_in client;
    int threadno = 0;
    fd_set readFDs;
    fd_set masterFDs;
    int i, j, maxSocket, ready, newClient, clientAction;

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    struct timeval timeout;
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;

    int client_len = sizeof(client); 

    // Open socket
    if (sockfd < 0) 
        fprintf(stderr, "Error opening socket.\n");

    // Bind socket to port
    if (bind(sockfd, (struct sockaddr *)&server, sizeof(server)) < 0) {
        fprintf(stderr, "Error binding socket.\n");
        exit(1);
    }

    fprintf(stderr, "Opened socket and bound to port %d.\n", server.sin_port);

    // Listen on port
    listen(sockfd, 10);

    FD_ZERO(&masterFDs);
    FD_ZERO(&readFDs);
    FD_SET(sockfd, &masterFDs);
    maxSocket = sockfd + 1;

    fprintf(stderr, "Main Socket == %d\n", sockfd);

    while(1) {
        //No time out for now
        readFDs = masterFDs;

        ready = select(maxSocket, &readFDs, NULL, NULL, NULL);

        if (ready < 0) {
            fprintf(stderr, "Error in Select()\n", NULL);
        } 

        //Accept new connection and add to fd_set
        if(FD_ISSET(sockfd, &readFDs)) {
            newClient = accept(sockfd, (struct sockaddr *) &client, &client_len);
            if(newClient == -1){
                perror("Server error in accept");
            }
            FD_SET(newClient, &fds);
            if((newClient + 1) > maxSocket) {
                maxSocket = newClient + 1;
                fprintf(stderr, "Adding new client sockfd === %d\n", newClient);
            }
        }

        for(i = 0; i<maxSocket; i++) {
            if(FD_ISSET( i, &readFDs)) {
                //Accept new connection and add to fd_set
                if(i == sockfd){
                    newClient = accept(sockfd, (struct sockaddr *) &client, &client_len);
                    if(newClient == -1){
                        perror("Server error in accept");
                    }
                    FD_SET(newClient, &masterFDs);
                    if((newClient + 1) > maxSocket) {
                        maxSocket = newClient + 1;
                    }
                    fprintf(stderr, "Adding new client sockfd ===== %d\n", newClient);
                } else{
                    fprintf(stderr, "Handling client : %d\n", i);
                    clientAction = handle_client(i);
                    //Will use clientAction in the future for determining
                    //if a client is done or still has data
                    FD_CLR(i, &masterFDs);
                }
            } else {
                for(j=0; j<maxSocket; j++){
                    if(j!=sockfd && FD_ISSET(j, &masterFDs)) {
                        fprintf(stderr, "Handling client in J LOOP: %d\n", j);
                        clientAction = handle_client(i);
                        //Will use clientAction in the future for determining
                        //if a client is done or still has data
                        FD_CLR(i, &masterFDs);
                    }
                }
            }
        }

    }

    fprintf(stderr, "Select\n");
    return 0;
}

//For now, using a simple read all data from single client 
//May alter in the future to allow for reading from multiple client at a time
int handle_client(int clientsockfd) {
    char buffer[1000];
    int readbytes; 

    // Initial read
    if ((readbytes = read(clientsockfd, buffer, sizeof(buffer))) < 0) {
        fprintf(stderr, "Read from client failed.\n");  
    }
    
    // If there was something left to read, go back for more until none is left
    while (readbytes > 0) {
        if ((readbytes = read(clientsockfd, buffer, sizeof(buffer))) < 0) {
            fprintf(stderr, "Read from client failed.\n");
        }
    }
 
    // Close socket
    int closeValue = close(clientsockfd);
    if(closeValue == -1) {
        perror("Error closing client socket:::: ");
    }
    fprintf(stderr, "Request completed, client connection %d closed.\n", clientsockfd);
    return 0;
}

int main(int argc, char **argv) {
    if (argc != 3) {
       printf("Usage: %s <server_implementation> <port>\n", argv[0]);
       printf("Server implementation options: \nthread \npolling_aio \npolling_read \nselect\n");
       return -1;
    }
    
    port = atoi(argv[2]);

    if (strcmp(argv[1], "thread") == 0) {
       thread_impl();
    }
    else if (strcmp(argv[1], "polling_aio") == 0) {
       aio_read = 1;
       polling_impl();
    }
    else if (strcmp(argv[1], "polling_read") == 0) {
       aio_read = 0;
       polling_impl();
    }
    else {
       select_impl();
    }
    
    return 0;
}
