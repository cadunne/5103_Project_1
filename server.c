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
#define AIO_LIST_SIZE 1024 * 10

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
  struct timeval start;
} threadarg;

typedef struct time_tuple{
    struct timeval start;
    struct timeval end;
    int bytes_read;
} time_tuple;

struct sockaddr_in server;

struct timeval start;
float total;

float totaltime = 0.0;
long int allread = 0;
pthread_mutex_t lock;

// Function that the threads execute
void *handle_request(void *arg) {
	
	// 1KB Buffer to read into 
	char buffer[BUF_SIZE];
	int readbytes; 
	threadarg myarg;
	myarg = *((threadarg*) arg);
	int clientfd = myarg.clientfd;
	int i = myarg.threadnum;
	long int totalbytes;

	// Timing per thread
	struct timeval endtime;
	struct timeval starttime = myarg.timer;
  struct timeval overall_start = myarg.start;
	

  	// Initial read
	if ((readbytes = read(clientfd, buffer, sizeof(buffer))) < 0) {
		fprintf(stderr, "Read from client failed.\n");   	
	}
  totalbytes += (long int) readbytes;

  	// If there was something left to read, go back for more until none is left
	while (readbytes > 0) {
		if ((readbytes = read(clientfd, buffer, sizeof(buffer))) < 0) {
			fprintf(stderr, "Read from client failed.\n");
		}	
    totalbytes += (long int) readbytes;
	}
 
  	// Close socket
	close(clientfd);

	// Stop timer
	gettimeofday(&endtime, NULL);

  float thread_time = ((float)(((endtime.tv_sec - starttime.tv_sec)*1000000) + (endtime.tv_usec - starttime.tv_usec))) / 1000000;
  float mb = ((float) totalbytes) / 1048576;
  
  fprintf(stderr, "Thread %d took %f seconds to read %f MB.\nThread throughput: %f MB/sec\n\n", i, mb, thread_time, mb/thread_time);

  	// Exit thread
	pthread_exit(0);
}

int thread_impl() {

   	int clientsockfd;
   	struct sockaddr_in client;
	int threadno = 0;
	struct timeval total_time;
  struct timeval orig_start;

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
    	listen(sockfd, 124);

        int client_len = sizeof(client);    

    	while ((clientsockfd = accept(sockfd, (struct sockaddr *) &client, (socklen_t *)&client_len)))
    	{
		// Create thread's timer
		struct timeval thread_timer;
		gettimeofday(&thread_timer, NULL);

    if (threadno == 0) {
        orig_start = thread_timer;
    }
        	// Create thread object
        	pthread_t client_thread;        

        	// Allocate thread arguments
		threadarg *thread_arg = malloc(sizeof(*thread_arg));
		thread_arg->clientfd = clientsockfd;
		thread_arg->threadnum = threadno;
		thread_arg->timer = thread_timer; 
    thread_arg->start = orig_start;       

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
    double total_throughput = 0.0;
    
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
                    time_list[i].bytes_read += ret;
                    aio_read(aiocbp);
                }

                else{
                    //Read totally finished
                    //printf ("SUCCESS on %d\n", i);
                                        
                    //cleanup:
                    close(aiocbp->aio_fildes);                    
                    completed[i] = 1;
                    
                    //timing and throughput:
                    gettimeofday(&(time_list[i].end), NULL);
                    total_finished++;
                    
                    suseconds_t startDouble = (time_list[i].start.tv_sec * 1000000 + time_list[i].start.tv_usec);
                    suseconds_t endDouble = (time_list[i].end.tv_sec * 1000000 + time_list[i].end.tv_usec);
                    suseconds_t timeDiff = endDouble - startDouble;
                    
                    double single_client_throughput = (double)time_list[i].bytes_read / timeDiff * (1000000.0 / (1024*1024));    
                    
                    total_time += timeDiff;
                    total_throughput += single_client_throughput;
                    
                    printf("Client #%d took %ldusec. Bytes read: %d. Throughput: %fMB/s\n",
                        i, timeDiff, time_list[i].bytes_read, single_client_throughput);
                    printf("---Avg. time: %ldusec, Avg. throughput: %fMB/sec\n", total_time/total_finished, total_throughput/total_finished);
                }
            }
        }
    }
    return 0;
}

int polling_impl_read() {

    int clientsockfd;
    int client_len;
    int socks_used = 0;
    struct sockaddr_in client;
    struct aiocb aiocb;
    struct aiocb *aiocbp;
    int i = 0;
    int total_finished = 0; //for calculating average time
    suseconds_t total_time = 0.0; //for caluclating average time
    double total_throughput = 0.0;
    
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
        time_list[i].bytes_read = 0;
        }
    
    while(1) {
        /////////////////////////////////STAGE 1: Accept / Connect / Read if possible///////////////////////////////////////////////////
        
        //only spawn new sockets if we have some available (capping to 1000)
        if(socks_used <= 1000){
            clientsockfd = accept(sockfd, (struct sockaddr *) &client, (socklen_t *) &client_len);
        }
        else{
            printf("Waiting for socks to open...\n");
            clientsockfd = -1;
        }
        
        if(clientsockfd < 0){
            //no new connections
        }
        else{
            
            socks_used++;
        
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
                if(errno == 11){
                    //no problem, 'EAGAIN'
                }
                else{
                    printf("Error with aio_read, %d\n", (errno));
                }
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
                    time_list[i].bytes_read += status;
                    read(aiocbp->aio_fildes, aiocbp->aio_buf, aiocbp->aio_nbytes);
            }
            else{
                //Read totally finished
                //printf ("SUCCESS on %d\n", i);
                                    
                //cleanup:
                close(aiocbp->aio_fildes);                    
                completed[i] = 1;
                socks_used--;
                
                //timing:
                gettimeofday(&(time_list[i].end), NULL);
                total_finished++;
                
                suseconds_t startDouble = (time_list[i].start.tv_sec * 1000000 + time_list[i].start.tv_usec);
                suseconds_t endDouble = (time_list[i].end.tv_sec * 1000000 + time_list[i].end.tv_usec);
                suseconds_t timeDiff = endDouble - startDouble;
                
                double single_client_throughput = (double)time_list[i].bytes_read / timeDiff * (1000000.0 / (1024*1024));    
                    
                total_time += timeDiff;
                total_throughput += single_client_throughput;
                
                printf("Client #%d took %ldusec. Bytes read: %d. Throughput: %fMB/s\n",
                    i, timeDiff, time_list[i].bytes_read, single_client_throughput);
                printf("---Avg. time: %ldusec, Avg. throughput: %fMB/sec\n", total_time/total_finished, total_throughput/total_finished);
            }
        }
    }
    return 0;
}

int handle_client(int clientsockfd) {
    //Reading 1MB at a time
    char buffer[1048576];
    int readbytes, i, closeValue;

    if ((readbytes = read(clientsockfd, buffer, sizeof(buffer))) < 0) {
        //Client still reading
        return 0;
    }

    if (readbytes == 0) {
        closeValue = close(clientsockfd);
        if(closeValue == -1) {
            perror("Error closing client socket\n");
        }
        return -1;
    }

    return readbytes;
}

int select_impl() {

    int clientsockfd;
    struct sockaddr_in client;
    int threadno = 0;
    fd_set readFDs;
    fd_set masterFDs;
    int i, maxSocket, ready, newClient, bytesRead, flags;

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    int client_len = sizeof(client); 

    // Open socket
    if (sockfd < 0) 
        fprintf(stderr, "Error opening socket.\n");

    // Bind socket to port
    if (bind(sockfd, (struct sockaddr *)&server, sizeof(server)) < 0) {
        fprintf(stderr, "Error binding socket.\n");
        exit(1);
    }

    fprintf(stderr, "Opened socket and bound to port %d.\n", port);

    // Listen on port --- 128 is the max for Linux and Unix
    listen(sockfd, 128);

    // Set up for file descriptors FD_SETs
    FD_ZERO(&masterFDs);
    FD_ZERO(&readFDs);
    FD_SET(sockfd, &masterFDs);
    maxSocket = sockfd + 1;

    // Storage for timing connections
    int finishedConnections = 0;
    struct time_tuple connectionInfo;
    struct time_tuple fdInfo[1024];
    suseconds_t total_time = 0.0;
    double total_throughput = 0.0;
    int total_written_bytes = 0;

    // Number of open file desciptors --- 1024 is max for Linux and Unix
    int maxOpenFDs = 1024;
    int openFDs = 4;

    while(1) {

        readFDs = masterFDs;

        ready = select(maxSocket, &readFDs, NULL, NULL, NULL);

        if (ready < 0) {
            fprintf(stderr, "Error in Select()\n", NULL);
        }

        for(i = (sockfd+1); i<(maxSocket+1); i++) {
            // Client reading
            if(FD_ISSET( i, &readFDs)) {
                bytesRead = handle_client(i);

                if(bytesRead == -1) {
                    // Client is finished remove 
                    FD_CLR(i, &masterFDs);

		            // Get end time
                    gettimeofday(&fdInfo[i].end, NULL);
                    openFDs--;

                    // Add connection information 
                    connectionInfo = fdInfo[i];
                    finishedConnections++;

                    // Print information
                    suseconds_t startDouble = (connectionInfo.start.tv_sec * 1000000 + connectionInfo.start.tv_usec);
                    suseconds_t endDouble = (connectionInfo.end.tv_sec * 1000000 + connectionInfo.end.tv_usec);
                    suseconds_t timeDiff = endDouble - startDouble;

                    double single_client_throughput = (double)connectionInfo.bytes_read / timeDiff * (1000000.0 / (1024*1024));    

                    total_time += timeDiff;
                    total_throughput += single_client_throughput;

                    printf("Client #%d took %ldusec. Bytes read: %d. Throughput: %fMB/s\n",
                    finishedConnections, timeDiff, connectionInfo.bytes_read, single_client_throughput);
                    printf("---Avg. time: %ldusec, Avg. throughput: %fMB/sec\n", total_time/finishedConnections, total_throughput/finishedConnections);

                } else {
                    // Update bytes read 
                    fdInfo[i].bytes_read += bytesRead;
                    total_written_bytes += bytesRead;
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

                // Adjust flags
		        flags = fcntl(newClient, F_GETFL, 0);
		        fcntl(newClient, F_SETFL,  flags | O_NONBLOCK);

		        //Get start time
                gettimeofday(&fdInfo[newClient].start, NULL);
                FD_SET(newClient, &masterFDs);
                openFDs++;

                // Clean out connectionInfo
                fdInfo[newClient].bytes_read = 0;

                // Update maxSocket 
                if((newClient + 1) > maxSocket) {
                    maxSocket = newClient + 1;
                }
            }
        }
    }

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
