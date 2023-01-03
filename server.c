#include "segel.h"
#include "request.h"

// 
// server.c: A very, very simple web server
//
// To run:
//  ./server <portnum (above 2000)>
//
// Repeatedly handles HTTP requests sent to this port number.
// Most of the work is done within routines written in request.c
//

// HW3: Parse the new arguments too
void getargs(int *port, int argc, char *argv[], int* numofthreads,int* queuesize, char* sched)
{
    if (argc < 2) {
	fprintf(stderr, "Usage: %s <port>\n", argv[0]);
	exit(1);
    }
    *port = atoi(argv[1]);
    *numofthreads =atoi(argv[2]);
    *queuesize =atoi(argv[3]);
    strcpy(sched,argv[4]);
}


int main(int argc, char *argv[])
{
    int listenfd, connfd, port, clientlen, numofthreads, queuesize;
    char sched[10];
    struct sockaddr_in clientaddr;

    getargs(&port, argc, argv, &numofthreads, queuesize, sched);

    //
    // HW3: Create some threads...
    //

    listenfd = Open_listenfd(port);
    while (1) {
	clientlen = sizeof(clientaddr);
	connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);

	// 
	// HW3: In general, don't handle the request in the main thread.
	// Save the relevant info in a buffer and have one of the worker threads 
	// do the work. 
	// 
	requestHandle(connfd);

	Close(connfd);
    }

}


    


 
