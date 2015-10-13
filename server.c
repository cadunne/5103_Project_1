/**********************************/
/****** CSCI 5103 Project 1 *******/
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

int aio_read;
int port;
int sockfd;

typedef struct {
    short		sin_family;
    unsigned short	sin_port;
    struct in_addr	sin_addr;
    char		sin_zero[8];
} sockaddr_in;

struct sockaddr_in server;

int thread_impl() {
    
    int clientsockfd;
    struct sockaddr_in client;

    fprintf(stderr, "Thread\n");
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

    fprintf(stderr, "Opened socket and bound to port %d.\n");
    
    // Listen on port
    listen(sockfd, 5);
    
    int client_len = sizeof(client);    

    while (clientsockfd = accept(sockfd, (struct sockaddr *) &client, &client_len))
    {
        // Create thread
        pthread_t client_thread;
        
        if (pthread_create(&client_thread, NULL, handle_request, clientsockfd) < 0) {
            fprintf(stderr, "Failed to create thread");
        }
        if (pthread_detach(&client_thread) < 0) {
            fprintf(stderr, "Failed to detach thread.");
        }

    }

    return 0;

}

int handle_request(int clientfd) {
    
}

int polling_impl() {
    fprintf(stderr, "Polling\n");
    if (aio_read) {

    }
    else {

    }
    return 0;
}

int select_impl() {
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
    
    fprintf(stderr, "Port: %d\n", port);

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
