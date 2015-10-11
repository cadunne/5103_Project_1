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
    
    client_len = sizeof(client);    

    while (clientsockfd = accept(sockfd, (struct sockaddr *) &client, &client_len))
    {
       //create thread
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

        while ((non-blocking)listen for clients to get connections)
        {
            //IF new client, do all adding, setup, etc
                //change flags so it's non-blocking http://stackoverflow.com/questions/914463/in-c-how-to-make-a-file-descriptor-blocking
                //For Karel: look at F_NOTIFY
                //add to gFD_SET


            //non-blocking call to handleConnectionMethod (different for each implementation)
        }


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
