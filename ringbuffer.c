/*
 * ringbuffer.c
 *
 *  Created on: 2016/11/10
 *      Author: heyong
 */

#include "stdtypes.h"
#include "ringbuffer.h"

int16_t rb_remaining_data(ringbuffer_t *rb)
{
  if(rb->w_pos > rb->r_pos)
    return rb->w_pos - rb->r_pos;
  else {
    return rb->size - (rb->r_pos - rb->w_pos);
  }
}

int16_t rb_available_space(ringbuffer_t *rb)
{
  return rb->size - rb_remaining_data(rb);
}

void ringbuffer_init(ringbuffer_t *rb, char *buff, uint16_t size)
{
  rb->buf = buff;
  rb->size = size;
  rb->r_pos = -1;
  //rb->w_pos = -1;
  rb->w_pos = -1-rb->size;
}

int16_t rb_write(ringbuffer_t *rb, char *buff, uint16_t len)
{
  uint16_t i=0;
  if(rb_available_space(rb) < len)
    return -1;

  if(rb->r_pos == -1) {
    rb->r_pos = rb->w_pos = 0;
  }
  while(i < len) {
    rb->buf[(rb->w_pos++)%rb->size] = buff[i++];
  }

  rb->w_pos = rb->w_pos % rb->size;
  //if(rb->w_pos == rb->r_pos)

  return 0;
}

int16_t rb_read(ringbuffer_t *rb, char *buff, uint16_t len)
{
  int16_t i=0;
  if(rb_remaining_data(rb) < len)
    return -1;

  while(i < len) {
    buff[i++] = rb->buf[(rb->r_pos++)%rb->size];
  }

  rb->r_pos = rb->r_pos % rb->size;
  if(rb->r_pos == rb->w_pos) {
    // empty
    rb->r_pos = -1;
    rb->w_pos = -1-rb->size;
  }

  return 0;
}

int16_t rb_read_prepare(ringbuffer_t *rb, char *buff, uint16_t len)
{
  int16_t i=0, r_pos;
  if(rb_remaining_data(rb) < len)
    return -1;

  r_pos = rb->r_pos;
  while(i < len) {
    buff[i++] = rb->buf[(r_pos++)%rb->size];
  }

  return 0;
}

int16_t rb_read_commit(ringbuffer_t *rb, uint16_t len)
{
  if(rb_remaining_data(rb) < len)
    return -1;

  rb->r_pos = (rb->r_pos + len) % rb->size;

  if(rb->r_pos == rb->w_pos) {
    // empty
    rb->r_pos = -1;
    rb->w_pos = -1-rb->size;
  }

  return 0;
}

