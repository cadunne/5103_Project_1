/**********************************/
/****** CSCI 5103 Project 1 *******/
/*** OS Support for Concurrency ***/
/**********************************/
/**** Connor Dunne // dunne064 ****/
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
#include <aio.h>

#define BUF_SIZE 1024
#define AIO_LIST_SIZE 1024

int will_aio_read;
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
	struct timeval orig_start;
} threadarg;

typedef struct time_tuple{
    struct timeval start;
    struct timeval end;
} time_tuple;

struct sockaddr_in server;

struct timeval start;
float total;

// Function that the threads execute
void *handle_request(void *arg) {
	
	// 1KB Buffer to read into 
	char buffer[BUF_SIZE];
	int readbytes; 
	threadarg myarg;
	myarg = *((threadarg*) arg);
	int clientfd = myarg.clientfd;
	int i = myarg.threadnum;
	struct timeval first_start = myarg.orig_start;	

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
	//close(clientfd);

	// Stop timer
	gettimeofday(&endtime, NULL);

	fprintf(stderr, "Request completed by thread %d.\n Elapsed time: %d microseconds.\nClient connection closed.\n\n", i, (((endtime.tv_sec * 100000) + endtime.tv_usec) - ((starttime.tv_sec * 100000) + starttime.tv_usec)));

	if ((i+1) % 10 == 0) {
		fprintf(stderr, "---------------\nTotal time so far at conclusion of thread %d: %d microseconds.\n---------------\n\n", i,  (((endtime.tv_sec * 100000) + endtime.tv_usec) - ((first_start.tv_sec * 100000) + first_start.tv_usec)));
	}
	
  	// Exit thread
	pthread_exit(0);
}

int thread_impl() {

   	int clientsockfd;
   	struct sockaddr_in client;
	int threadno = 0;
	struct timeval original_start;

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

    	while ((clientsockfd = accept(sockfd, (struct sockaddr *) &client, (socklen_t *)&client_len)))
    	{
		// Create thread's timer
		struct timeval thread_timer;
		gettimeofday(&thread_timer, NULL);

		if (threadno % 100 == 0) {
			gettimeofday(&original_start, NULL);
		}

        	// Create thread object
        	pthread_t client_thread;        

        	// Allocate thread arguments
		threadarg *thread_arg = malloc(sizeof(*thread_arg));
		thread_arg->clientfd = clientsockfd;
		thread_arg->threadnum = threadno;
		thread_arg->timer = thread_timer;        
		thread_arg->orig_start = original_start;

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

int polling_impl_aio() {

    int clientsockfd;
    int client_len;
    struct sockaddr_in client;
    struct aiocb aiocb;
    struct aiocb *aiocbp;
    int i = 0;
    int total_finished = 0; //for calculating average time
    suseconds_t total_time = 0.0; //for caluclating average time
    
    char buf2[BUF_SIZE];

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    //make socker non-blocking, for 'accept'
    fcntl(sockfd, F_SETFL, O_NONBLOCK);

    client_len = sizeof(client); 

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
    listen(sockfd, 50);

    //Setup aio array, time array, completion array
    int aio_count = 0;
    int ret, err;
    
    struct aiocb aio_list[AIO_LIST_SIZE];
    struct time_tuple time_list[AIO_LIST_SIZE];
    
    int completed[AIO_LIST_SIZE];
    for(i=0; i<AIO_LIST_SIZE; i++){
        completed[i] = 0;
        time_list[i].start.tv_sec = 0;
        time_list[i].end.tv_sec = 0;
    }
    
    while(1) {
    
        /////////////////////////////////STAGE 1: Accept / Connect / Read if possible///////////////////////////////////////////////////
        
        clientsockfd = accept(sockfd, (struct sockaddr *) &client, (socklen_t *) &client_len);
        
        if(clientsockfd < 0){
            //no new connections
        }
        else{
            //printf("Setting up new clientSockFd: %d\n", clientsockfd);
            
            //Clear buff and struct
            memset(&buf2, 0, BUF_SIZE);
            memset(&aiocb, 0, sizeof(struct aiocb));
            aiocb.aio_fildes = clientsockfd;
            aiocb.aio_buf = buf2;
            aiocb.aio_nbytes = BUF_SIZE;

            //update arrays
            aio_list[aio_count] = aiocb;
            gettimeofday(&(time_list[aio_count].start), NULL);
            
            aio_count++;

            if (aio_count > AIO_LIST_SIZE){
                printf("Too many connections for AIO_LIST_SIZE!");
            }
            
            //start download
            if(aio_read(&aio_list[aio_count-1]) != 0) {
                printf("Error with aio_read\n");
            }
            
        }

        /////////////////////////////STAGE 2: Iterate through aiocb's, see if any have completed//////////////////////////////////
        
        for(i = 0; i < aio_count; i++){
            
            if (completed[i] == 1){
                //already completed read
                continue;
            }
            
            aiocbp = &aio_list[i];
            err = aio_error(aiocbp);
            
            if (err != 0 && err != EINPROGRESS) {
              printf ("Error at aio_error() : %s\n", strerror (err));
            }
            
            else if (err == EINPROGRESS){
                //in progress, don't check return
            }
            
            else{
                //no error, aio_read is finished
                ret = aio_return(aiocbp);
                
                if (ret < 0) {
                  printf("Error at aio_return() : %s\n", strerror(errno));
                }
            
                else if (ret > 0){
                    //Partially finished; read next chunk into buffer
                    //printf("...Still reading, retval = %d\n", ret);
                    aio_read(aiocbp);
                }

                else{
                    //Read totally finished
                    //printf ("SUCCESS on %d\n", i);
                                        
                    //cleanup:
                    close(aiocbp->aio_fildes);                    
                    completed[i] = 1;
                    
                    //timing:
                    gettimeofday(&(time_list[i].end), NULL);
                    total_finished++;
                    
                    suseconds_t startDouble = (time_list[i].start.tv_sec * 1000000 + time_list[i].start.tv_usec);
                    suseconds_t endDouble = (time_list[i].end.tv_sec * 1000000 + time_list[i].end.tv_usec);
                    suseconds_t timeDiff = endDouble - startDouble;
                    
                    total_time += timeDiff;
                    
                    printf("Client #%d took %ldusec to finish. New avg. time: %ldusec\n", i, timeDiff, total_time/total_finished);
                    
                }
            }
        }
    }
    return 0;
}

int polling_impl_read() {

    int clientsockfd;
    int client_len;
    struct sockaddr_in client;
    struct aiocb aiocb;
    struct aiocb *aiocbp;
    int i = 0;
    int total_finished = 0; //for calculating average time
    suseconds_t total_time = 0.0; //for caluclating average time
    
    char buf2[BUF_SIZE];

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    //make socker non-blocking, for 'accept'
    
    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL,  flags | O_NONBLOCK);

    client_len = sizeof(client); 

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
    listen(sockfd, 50);

    //Setup aio array, time array, completion array
    int aio_count = 0;
    int ret, status;
    
    struct aiocb aio_list[AIO_LIST_SIZE];
    struct time_tuple time_list[AIO_LIST_SIZE];
    
    int completed[AIO_LIST_SIZE];
    for(i=0; i<AIO_LIST_SIZE; i++){
        completed[i] = 0;
        time_list[i].start.tv_sec = 0;
        time_list[i].end.tv_sec = 0;
    }
    
    while(1) {
    
        /////////////////////////////////STAGE 1: Accept / Connect / Read if possible///////////////////////////////////////////////////
        
        clientsockfd = accept(sockfd, (struct sockaddr *) &client, (socklen_t *) &client_len);
        
        if(clientsockfd < 0){
            //no new connections
        }
        else{
        
            int flags = fcntl(clientsockfd, F_GETFL, 0);
            fcntl(clientsockfd, F_SETFL,  flags | O_NONBLOCK);
            //printf("Setting up new clientSockFd: %d\n", clientsockfd);
            
            //Clear buff and struct
            memset(&buf2, 0, BUF_SIZE);
            memset(&aiocb, 0, sizeof(struct aiocb));
            aiocb.aio_fildes = clientsockfd;
            aiocb.aio_buf = buf2;
            aiocb.aio_nbytes = BUF_SIZE;

            //update arrays
            aio_list[aio_count] = aiocb;
            gettimeofday(&(time_list[aio_count].start), NULL);
            
            aio_count++;

            if (aio_count > AIO_LIST_SIZE){
                printf("Too many connections for AIO_LIST_SIZE!");
            }
            
            
            aiocbp = &(aio_list[aio_count-1]);
            
            
            //start download
            if(read(aiocbp->aio_fildes, aiocbp->aio_buf, aiocbp->aio_nbytes) != 0) {
                printf("Error with aio_read, %d\n", (errno));
            }
            
        }

        /////////////////////////////STAGE 2: Iterate through aiocb's, see if any have completed//////////////////////////////////
        
        for(i = 0; i < aio_count; i++){
            
            if (completed[i] == 1){
                //already completed read
                continue;
            }
            
            aiocbp = &aio_list[i];
            
            status = read(aiocbp->aio_fildes, aiocbp->aio_buf, aiocbp->aio_nbytes);

            if (status < 0){
                //in progress, don't check return
            }
            else if (status > 0){
                //Partially finished; read next chunk into buffer
                read(aiocbp->aio_fildes, aiocbp->aio_buf, aiocbp->aio_nbytes);
            }
            else{
                //Read totally finished
                //printf ("SUCCESS on %d\n", i);
                                    
                //cleanup:
                close(aiocbp->aio_fildes);                    
                completed[i] = 1;
                
                //timing:
                gettimeofday(&(time_list[i].end), NULL);
                total_finished++;
                
                suseconds_t startDouble = (time_list[i].start.tv_sec * 1000000 + time_list[i].start.tv_usec);
                suseconds_t endDouble = (time_list[i].end.tv_sec * 1000000 + time_list[i].end.tv_usec);
                suseconds_t timeDiff = endDouble - startDouble;
                
                total_time += timeDiff;
                
                printf("Client #%d took %ldusec to finish. New avg. time: %ldusec\n", i, timeDiff, total_time/total_finished);
            }
        }
    }
    return 0;
}

int handle_client(int clientsockfd) {
    //Read data from client 10KB at a time
    //char buffer[4096];
    char buffer[1048576];
    int readbytes, i, closeValue;

    // Read 1KB ten times or until finished
    //for(i=0; i<10; i++) {
        if ((readbytes = read(clientsockfd, buffer, sizeof(buffer))) < 0) {
            fprintf(stderr, "Read from client failed.\n"); 
            readbytes = 0; 
        }

        if (readbytes == 0) {
            closeValue = close(clientsockfd);
            if(closeValue == -1) {
                perror("Error closing client socket\n");
            }
            return -1;
        }
    //}
    return 0;
}

int select_impl() {

    int clientsockfd;
    struct sockaddr_in client;
    int threadno = 0;
    fd_set readFDs;
    fd_set masterFDs;
    int i, maxSocket, ready, newClient, clientAction;

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

    //fprintf(stderr, "Opened socket and bound to port %d.\n", port);

    // Listen on port --- supposedly 128 is the max
    listen(sockfd, 128);

    // Set up for file descriptors
    FD_ZERO(&masterFDs);
    FD_ZERO(&readFDs);
    FD_SET(sockfd, &masterFDs);
    maxSocket = sockfd + 1;

    // Timing
    struct timeval starttime;
    struct timeval endtime;
    gettimeofday(&starttime, NULL);
    gettimeofday(&endtime, NULL);

    // Number of open file desciptors
    int maxOpenFDs = 256;
    int openFDs = 4;

    //fprintf(stderr, "Main Socket = %d\n", sockfd);

    while(1) {

        readFDs = masterFDs;

        ready = select(maxSocket, &readFDs, NULL, NULL, NULL);

        if (ready < 0) {
            fprintf(stderr, "Error in Select()\n", NULL);
        }

        for(i = (sockfd+1); i<(maxSocket+1); i++) {
            // Client reading
            if(FD_ISSET( i, &readFDs)) {
                clientAction = handle_client(i);

                if(clientAction == -1) {
                    // Client is finished remove
                    gettimeofday(&endtime, NULL);
                    FD_CLR(i, &masterFDs);
                    openFDs--;
                }
            }
        }

       if(FD_ISSET( sockfd, &readFDs)) {
            // Check if max file desciptor limit has been reached
            if (openFDs < maxOpenFDs) {
                // Accept new connection and add to FD_SET
                newClient = accept(sockfd, (struct sockaddr *) &client, (socklen_t *) &client_len);

                if(newClient == -1){
                    perror("Server error in accept");
                }

                gettimeofday(&starttime, NULL);
                FD_SET(newClient, &masterFDs);
                openFDs++;

                if((newClient + 1) > maxSocket) {
                    maxSocket = newClient + 1;
                }
                //fprintf(stderr, "Adding new client sockfd = %d\n", newClient);
            } else {
                //fprintf(stderr, "\n********TOO MANY FDS OPEN************\n\n");
            }
        }
    }

    fprintf(stderr, "Select\n");
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
        polling_impl_aio();
    }
    else if (strcmp(argv[1], "polling_read") == 0) {
        polling_impl_read();
    }
    else {
       select_impl();
    }
    
    return 0;
}
