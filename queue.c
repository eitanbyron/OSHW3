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