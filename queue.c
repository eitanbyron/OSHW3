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