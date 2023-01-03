#include "queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "segel.h"
#include "request.h"
#include <stdbool.h>
#include <math.h>

QueueResult blockHandler(WorkerPool wp, int element, struct timeval *arrival)
{
    while(getNumAvailableBuffers(wp)==0)
    {
        pthread_cond_wait(&wp->queue_full, &wp->lock_queue);
    }
    QueueResult res=QueueAdd(wp->pending, element, arrival);
    pthread_cond_signal(&wp->queue_empty);
    return res;
}

QueueResult dropTailHandler(int element)
{
    close(element);
    return QUEUE_SUCCESS;
}
QueueResult dropHeadHandler(WorkerPool wp, int element,  struct timeval *arrival)
{
    if(wp->pending->size==0)
    {
        close(element);
        return QUEUE_SUCCESS;
    }
    int fd=wp->pending->first->data;
    queueDropHead(wp->pending); //to complete
    close(fd);
    QueueResult res=QueueAdd(wp->pending, element, arrival);
    pthread_cond_signal(&wp->queue_empty);
   
    return res;
}

QueueResult dropRandomHandler(WorkerPool wp, int element,  struct timeval *arrival)
{
    int half_size = wp->pending->size/2;
    for(int i = 0 ; i < half_size; i++)
    {
        int fd=queueDropRandom(wp->pending);//to complete
        close(fd);
    }
    QueueResult res=QueueAdd(wp->pending, element, arrival);
    pthread_cond_signal(&wp->queue_empty);
   
    return res;
}

pthread_args argsCreate(WorkerPool wp, int id)
{
    pthread_args args = malloc(sizeof(*args));
    args->wp =wp;
    args->number_of_thread = id;
    return args;
}

bool initializer(WorkerPool wp)
{
    if (pthread_mutex_init(&wp->lock_queue,NULL) != 0)
    {
        fprintf(stderr,"pthread_mutex_init() error\n");
        return false;
    }
    pthread_cond_init(&wp->queue_full,NULL);  //always success (tutroyal 8)
    pthread_cond_init(&wp->queue_empty,NULL); //always success (tutroyal 8)
    return true;
}

void chooseHandling(WorkerPool wp, char* sched)
{
    if(strcmp(sched,"block")==0)
    {
        wp->overload_handler=OVERLOAD_BLOCK;
    }
    else if(strcmp(sched,"dt")==0)
    {
        wp->overload_handler=OVERLOAD_DROP_TAIL;
    }
    else if(strcmp(sched,"dh")==0)
    {
        wp->overload_handler=OVERLOAD_DROP_HEAD;
    }
    else if(strcmp(sched,"random")==0)
    {
        wp->overload_handler=OVERLOAD_DROP_RAND;
    }
}

void handleWrapper (int fd, struct timeval* arrival, struct timeval* dispatch, int thread_id)
{
    requestHandle(fd);
    close(fd);
}

thread_rout rout =&thread_routine;

WorkerPool PoolCreate (int number_of_threads, int queue_size, char* sched)
{
    WorkerPool wp = malloc(sizeof(*wp));
    if (!initializer(wp))
        exit(1);
    wp->running =0;
    wp->stat_request_handeled =0;
    wp->dyn_requests_hadeled =0;
    wp->numOfThreads = number_of_threads;
    wp->pending = QueueCreate();
    wp->handler = handleWrapper;
    wp->max_queue_size = queue_size;
    chooseHandling(wp, sched);
    wp->threads = malloc(sizeof(*wp->threads)*number_of_threads);
    for (int i=0; i<number_of_threads; i++)
    {
        pthread_args args = argsCreate(wp,i);
        pthread_create(&wp->threads[i],NULL, rout , args);
    }
    return wp;
}

QueueResult WorkerPoolDequeue( WorkerPool wp, int thread_num)
{
    if (wp == NULL)
        return QUEUE_NULL_ARGUMENT;
    int fd;
    struct timeval arrival;
    struct timeval pick;
    struct timeval result;
    pthread_mutex_lock(&wp->lock_queue);
    while (QueueRemoveHead(wp->pending,&fd,&arrival) == QUEUE_EMPTY)
    {
        pthread_cond_wait(&wp->queue_empty,&wp->lock_queue);
    }
    gettimeofday(&pick,NULL);
    timersub(&pick,&arrival,&result);
    wp->running++;
    pthread_mutex_unlock(&wp->lock_queue);
    wp->handler....; //TODO: HANDLER
    pthread_mutex_lock(&wp->lock_queue);
    wp->running--;
    pthread_cond_broadcast(&wp->queue_full);
    pthread_mutex_unlock(&wp->lock_queue);
    return QUEUE_SUCCESS;
}


QueueResult WorkerPoolEnqueue(WorkerPool wp, int element, struct timeval *arrival){
        if(wp==NULL)
    {
        return QUEUE_NULL_ARGUMENT;
    }
    QueueResult res=NULL;
    pthread_mutex_lock(&wp->lock_queue);
    if(wp->running+QueueGetSize(wp->pending)==wp->max_queue_size)
    {
        switch(wp->overload_handler)
        {
            case OVERLOAD_BLOCK:
                res= blockHandler(wp, element, arrival);
            
            case OVERLOAD_DROP_TAIL:
                res= dropTailHandler(element);
            
            case OVERLOAD_DROP_HEAD:
                res= dropHeadHandler(wp, element, arrival);
            
            case OVERLOAD_DROP_RAND: 
                res= dropRandomHandler(wp, element, arrival);
            
            //option for a default case
        }
        pthread_mutex_unlock(&wp->lock_queue);
        return res;

    }

}


void* thread_routine (pthread_args args)
{
    while(true)
    {
        workerPoolDequeue(args->wp, args->number_of_thread);
    }
}

// ******************************* Node Implementation *****************************************
Node NodeCreate(int data, struct timeval *arrival) {
    Node node = malloc(sizeof(node));
    if(node==NULL)
    {
        return NULL;
    }
    node->data = data;
    node->arrival = *arrival;
    // node->next = next;
    // new_node->prev=NULL;
    // if(next!=NULL)
    // {

    //     next->prev=new_node;
    // }
    return node;
}

Node getNodeByIndex(Queue queue, int index)
{
    if(queue==NULL || index > queue->size)
    {
        return NULL;
    }
    Node iter=queue->first;
    int i;
    for( i=0 ; i < queue->size ; i++)
    {
        if(i==index)
        {
            return iter;
        }
        if(iter->next==NULL) return NULL;
        iter=iter->next;
    }
    return NULL;
}

// ******************************* Queue Implementation *****************************************
Queue QueueCreate()
{
    Queue queue=malloc(sizeof(*queue));
    if(queue==NULL)
    {
        return NULL;
    }
    queue->head=0;
    queue->tail=0;
    queue->first=NULL;
    queue->last=NULL;
    queue->size=0;
    return queue;
}

void QueueDestroy(Queue queue)
{
    if(queue==NULL){
        reutrn;
    }
    if(queue->size==0){
        return;
    }
    Node curr=queue->first;
    Node prev=queue->first;
    while(curr!=NULL)
    {
        curr=curr->next;
        free(prev);
        prev=curr;
    }
}

int QueueGetSize(Queue queue)
{
    if(queue==NULL)
    {
        return -1;
    }
    return queue->size;
}
QueueResult QueueAdd (Queue queue,int element, struct timeval *arrival)
{
    if(queue==NULL)
    {
        return QUEUE_NULL_ARGUMENT;
    }
    Node new_node=NodeCreate(element,arrival);
    if(new_node==NULL)
    {
        return QUEUE_ADD_FAILED;
    }
    new_node->next=queue->first;
    new_node->prev=NULL;
    if(queue->first!=NULL)
    {
        queue->first->prev=new_node;
    }else{
        if(queue->size==0)
        {
            queue->last=new_node;
        }
    }
    queue->first=new_node;
    queue->size++;
    return QUEUE_SUCCESS;
}

void QueueDeleteByIndex(Queue queue, int index)
{
    if(queue==NULL)
    {
        return;
    }
    if(index==0)
    {
        QueueRemoveHead(queue,&to_remove);
        close(to_remove);
        return;
    }
    
    Node to_remove=getNodeByIndex(queue,index);
    if(to_remove==NULL)return;

    if(to_remove==queue->last)
    {
        queue->last=to_remove->prev;
        to_remove->prev->next=NULL;
    }else{
        to_remove->prev->next=to_remove->next;
        to_remove->next->prev=to_remove->prev;
    }

    close(to_remove->connection_descriptor);
    free(to_remove);
    queue->size--;
}

QueueResult QueueRemoveHead (Queue queue, int* fd, struct timeval* arrival)
{
    if (queue == NULL)
        return QUEUE_NULL_ARGUMENT;
    if (QueueGetSize(queue) == 0)
        return QUEUE_EMPTY;
    Node before_last =queue->last->prev;
    if (before_last != NULL)
    {
        *arrival = queue->last->arrival;
        *fd = queue->last->connection_descriptor;
        free (queue->last);
        before_last->next=NULL;
        queue->last=before_last;
    }
    else
    {
        *arrival =queue->first->arrival;
        *fd=queue->first->connection_descriptor;
        free(queue->first);
        queue->first=NULL;
        queue->last=NULL;
    }
    queue->size--;
    return QUEUE_SUCCESS;
}