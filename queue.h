#include <sys/time.h>

typedef struct Node_t* Node;
typedef struct Queue_t *Queue;
typedef struct WorkerPool_t *WorkerPool;
typedef struct pthread_args_t *pthread_args;

typedef void(*runHandler)(int, int*, int*, int*, struct timeval*, struct timeval*, int);
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

struct Queue_t { 
    struct Node_t* first;
    struct Node_t* last;
    int head;
    int tail;
    int size;
};

typedef enum Overload_t {
    OVERLOAD_BLOCK,
    OVERLOAD_DROP_TAIL,
    OVERLOAD_DROP_RAND,
    OVERLOAD_DROP_HEAD
} Overload;

struct Node_t {
    int data; 
    struct timeval arrival;
    struct Node_t* next;
    struct Node_t* prev;
};

struct Queue_t { 
   struct Node_t* first;
   struct Node_t* last;
   int size;
};

struct WorkerPool_t {
    Queue pending;
    pthread_t* threads;
    pthread_cond_t queue_empty;
    pthread_cond_t queue_full;
    pthread_mutex_t lock_queue;
    int running;
    int max_queue_size;
    int numOfThreads;
    runHandler handler;
    Overload overload_handler;
};

// Node Functions
Node NodeCreate(int data, struct timeval* arrival); 
Node getNodeByIndex(Queue queue, int index);
// Queue Functions
Queue QueueCreate();
void QueueDestroy(Queue queue);
int QueueGetSize(Queue queue);
QueueResult QueueAdd (Queue queue,int element, struct timeval* arrival);
struct timeval QueueRemoveHead (Queue queue, int* head);
//void QueuePrint(Queue queue);


WorkerPool WorkerPoolCreate(int numOfThreads, int max_queue_size, char* sched);


QueueResult blockHandler();
QueueResult dropTailHandler();
QueueResult dropHeadHandler();
QueueResult dropRandomHandler();