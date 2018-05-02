/*
 * port.c
 *
 *  Created on: 2016/11/21
 *      Author: heyong
 */

#include "port.h"

#if defined(APP_ON_CONSOLE)

#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

void os_sleep(uint32_t ms) {
  usleep(ms*1000);
}

uint32_t os_get_time() {
  struct timeval tv;
  gettimeofday(&tv,0);
  return (tv.tv_sec&0xFFFFF)<<12 | (tv.tv_usec/1000);
}

uint32_t os_get_duration_ms(uint32_t prev) {
  struct timeval tv;
  uint32_t c_s, c_ms, p_s, p_ms, diffms; // max 49 days

  gettimeofday(&tv,0);
  p_s = (prev>>12);
  p_ms = (prev&0xFFF);
  c_s = (tv.tv_sec&0xFFFFF);
  c_ms = (tv.tv_usec/1000);

  if(c_ms >= p_ms) {
    diffms = c_ms-p_ms;
    if(c_s >= p_s) {
      diffms += (c_s-p_s)*1000;
    } else {
      // 136 years later since 1970-01-01
    }
  } else {
    diffms = c_ms + 1000 - p_ms;
    c_s -= 1;
    if(c_s >= p_s) {
      diffms += (c_s-p_s)*1000;
    } else {
      // 136 years later since 1970-01-01
    }
  }
  //dbg_printf("diff:%x\n", diffms);
  return diffms;
}

void* mem_alloc (uint32_t size) {
  return malloc(size);
}

void mem_free (void *p) {
  free(p);
}

#elif defined(APP_ON_MTK_NUCLEUS)

#include "drv_comm.h"

void os_sleep(uint32_t ms) {
  kal_sleep_task((uint32_t)(ms+4)/KAL_MILLISECS_PER_TICK);
}

uint32_t os_get_time() {
  return drv_get_current_time();
}

uint32_t os_get_duration_ms(uint32_t prev) {
  return drv_get_duration_ms(prev);
}

void* mem_alloc (uint32_t size) {
  return get_ctrl_buffer(size);
}

void mem_free (void *p) {
  free_ctrl_buffer(p);
}

void dbg_printf_array(char_t *str, uint8_t *ar, uint16_t len) {
    char_t *print_str;
    uint16_t i,n;

    print_str = mem_alloc(128);
    n = sprintf(print_str, "%s", str);
    for(i=0; i<len; i++) {
        n += sprintf(print_str+n, "%x-", ar[i]);
    }
    print_str[n-1] = '\0';
    dbg_printf(print_str);
    mem_free(print_str);
}

#else
#endif
