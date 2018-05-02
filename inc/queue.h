/*
 * queue.h
 *
 *  Created on: 2016Äê11ÔÂ15ÈÕ
 *      Author: heyong
 */

#ifndef _QUEUE_H_
#define _QUEUE_H_


//#define QUEUE_TYPE  void*

typedef struct _queue {
	QUEUE_TYPE *buf;
	int16_t qhead;
	int16_t qtail;
	int16_t size;
} queue_t;

#define queue_full(qu) (((qu)->qtail+1)%qu->size == (qu)->qhead)
#define queue_empty(qu) ((qu)->qhead == -1 && (qu)->qtail == -1)

void queue_init(queue_t *qu, QUEUE_TYPE *buf, uint16_t size);
int32_t enqueue_tail(queue_t *qu, QUEUE_TYPE *p);
int32_t enqueue_head(queue_t *qu, QUEUE_TYPE *p);
QUEUE_TYPE dequeue(queue_t *qu);
QUEUE_TYPE get_queue_head(queue_t *qu);
void dequeue_head_pointer(queue_t *qu);


#endif /* INC_QUEUE_H_ */
