/*
 * queue.c
 *
 *  Created on: 2016/11/10
 *      Author: heyong
 */

#include <stdio.h>
#include "stdtypes.h"

#define QUEUE_TYPE  void*
#include <queue.h>

void queue_init(queue_t *qu, QUEUE_TYPE *buf, uint16_t size)
{
  qu->qhead = -1;
  qu->qtail = -1;
  qu->buf = buf;
  qu->size = size;
}

int32_t enqueue_tail(queue_t *qu, QUEUE_TYPE *p)
{
  if(queue_full(qu))
    return -1;
  qu->qtail = (qu->qtail+1)%qu->size;
  qu->buf[qu->qtail] = *p;
  if(qu->qhead == -1)
  {
	  qu->qhead = qu->qtail;
  }
  return 0;
}

int32_t enqueue_head(queue_t *qu, QUEUE_TYPE *p)
{
  if(queue_full(qu))
    return -1;
  qu->qhead = (qu->qhead-1+qu->size)%qu->size;
  qu->buf[qu->qhead] = *p;
  if(qu->qtail == -1)
  {
	  qu->qtail = qu->qhead;
  }
  return 0;
}

QUEUE_TYPE get_queue_head(queue_t *qu)
{
  if(queue_empty(qu))
    return NULL;
  return qu->buf[qu->qhead];
}

void dequeue_head_pointer(queue_t *qu)
{
  if(qu->qhead == qu->qtail)
	  qu->qhead = qu->qtail = -1;
  else
	  qu->qhead = (qu->qhead+1)%qu->size;
}

QUEUE_TYPE dequeue(queue_t *qu)
{
  QUEUE_TYPE temp;
  temp = get_queue_head(qu);
  if(temp != NULL) {
    dequeue_head_pointer(qu);
  }

  return temp;
}

