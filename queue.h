#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "segel.h"
#include "request.h"
#include <stdbool.h>
#include <math.h>

typedef struct Node_t* Node;
typedef struct Queue_t *Queue;
typedef struct WorkerPool_t *WorkerPool;
typedef struct pthread_args_t *pthread_args;

typedef void(*runHandler)(int, WorkerPool, int, struct timeval*, struct timeval*);
typedef enum QueueResult_t {
    QUEUE_SUCCESS,
    QUEUE_NULL_ARGUMENT,
    QUEUE_ITEM_ALREADY_EXISTS,
    QUEUE_ITEM_DOES_NOT_EXISTS,
    QUEUE_EMPTY,
    QUEUE_ADD_FAILED
} QueueResult;


 struct Node_t {
    int connection_descriptor;
    struct timeval arrival;
    struct Node_t* next;
    struct Node_t* prev;
};

typedef enum Overload_t {
    OVERLOAD_BLOCK,
    OVERLOAD_DROP_TAIL,
    OVERLOAD_DROP_RAND,
    OVERLOAD_DROP_HEAD
} Overload;



struct Queue_t { 
   struct Node_t* first;
   struct Node_t* last;
   int size;
};

struct pthread_args_t {
    WorkerPool wp;
    int number_of_thread;
};

struct WorkerPool_t {
    Queue pending;
    pthread_t* threads;
    pthread_cond_t queue_empty;
    pthread_cond_t queue_full;
    pthread_mutex_t lock_queue;
    int running;
    int static_counter;
    int dynamic_counter;
    int request_counter;
    int max_queue_size;
    int numOfThreads;
    runHandler handler;
    Overload overload_handler;
};

WorkerPool WorkerPoolCreate(int numOfThreads, int max_queue_size, char* sched);


// Node Functions
Node NodeCreate(int data, struct timeval* arrival); 
Node getNodeByIndex(Queue queue, int index);
// Queue Functions
Queue QueueCreate();
void QueueDestroy(Queue queue);
int QueueGetSize(Queue queue);
QueueResult QueueAdd (Queue queue,int element, struct timeval* arrival);
QueueResult QueueRemoveHead (Queue queue, int* fd, struct timeval* arrival);
//void QueuePrint(Queue queue);
void QueueDeleteByIndex(Queue queue, int index);
QueueResult WorkerPoolAddConnection(WorkerPool wp, int fd,struct timeval *arrival);
QueueResult WorkerPoolEnqueue(WorkerPool wp, int element, struct timeval *arrival);
QueueResult WorkerPoolDequeue( WorkerPool wp, int thread_num);

QueueResult blockHandler(WorkerPool wp, int element, struct timeval *arrival);
QueueResult dropTailHandler(int element);
QueueResult dropHeadHandler(WorkerPool wp, int element, struct timeval *arrival);
QueueResult dropRandomHandler(WorkerPool wp, int element, struct timeval *arrival);
void* thread_routine(pthread_args args);
typedef void*(*thread_rout)(void*);
