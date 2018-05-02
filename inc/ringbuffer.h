#ifndef _RINGBUFFER_H_
#define _RINGBUFFER_H_

#include "stdtypes.h"

typedef struct {
  char *buf;
  int16_t r_pos;
  int16_t w_pos;
  int16_t size;
} ringbuffer_t;

#define rb_full(rb) ((rb->w_pos+1)%rb->size == rb->r_pos)
#define rb_empty(rb) (rb->r_pos == -1 && rb->w_pos == -1)


int16_t rb_remaining_data(ringbuffer_t *rb);

int16_t rb_available_space(ringbuffer_t *rb);

void ringbuffer_init(ringbuffer_t *rb, char *buff, uint16_t size);

int16_t rb_write(ringbuffer_t *rb, char *buff, uint16_t len);

int16_t rb_read(ringbuffer_t *rb, char *buff, uint16_t len);

int16_t rb_read_prepare(ringbuffer_t *rb, char *buff, uint16_t len);

int16_t rb_read_commit(ringbuffer_t *rb, uint16_t len);

#endif
