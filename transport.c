#include "stdtypes.h"
#include "as60x.h"
#include "transport.h"

#define SEND_BUFF_SIZE      (128)
#define RECV_BUFF_SIZE      (128)
#define TX_PKT_QUEUE_SIZE   (4)

#define SERIAL_SINGLE_RW_LEN  64

static int32_t push_pkt_to_tx_ringbuffer(as60x_cmd_packet_t *pkt);
static int32_t read_data_to_rx_ringbuffer();


static char *recv_buff=NULL;
static char *send_buff=NULL;
//static int16_t recv_r, recv_w, send_r, send_w;
static ringbuffer_t send_rb, recv_rb;

#undef QUEUE_TYPE
#define QUEUE_TYPE  as60x_cmd_packet_t*
#include <queue.h>

static as60x_cmd_packet_t* pkt_queue_buffer[TX_PKT_QUEUE_SIZE];
static queue_t pkt_queue; // for write

static int32_t handle=-1;
static int8_t speedup_read=0;
//static char debug_buf[256];
static uint8_t wait_for_bootup_flag=0;
static uint8_t transport_suspend=0;

#define word_swap_bytes(x)  ((x&0xff)<<8 | (x&0xff00)>>8)

static uint16_t pkt_checksum(as60x_cmd_packet_t *pkt) {
    uint16_t sum=0, i;
    sum += (pkt->type + (pkt->length&0xff) + (pkt->length>>8) + pkt->cmd);
    for(i=0; i<pkt->cmd_para.len; i++)
        sum += pkt->cmd_para.data[i];
    //dbg_printf("pkt_checksum:%x->%x\n", sum, word_swap_bytes(sum));
    return word_swap_bytes(sum);
}

static bool verify_pkt_checksum(as60x_recv_packet_common_t *pkt) {
    uint16_t pkt_sum, sum=0, i;
    sum += (pkt->type + (pkt->length&0xff) + (pkt->length>>8));
    for(i=0; i<(pkt->length-PKT_CHKSUM_SIZE); i++)
        sum += pkt->data[i];

    pkt_sum = pkt->data[pkt->length-PKT_CHKSUM_SIZE]<<8 | pkt->data[pkt->length-PKT_CHKSUM_SIZE+1];
    //dbg_printf("verify_pkt_checksum:(%x = %x) %x %x %x %x\n",
    //    sum, pkt_sum, pkt->type, (pkt->length&0xff), (pkt->length>>8), pkt->data[0]);
    return sum == pkt_sum;
}

void as60x_transport_wait_bootflag_on() {
    wait_for_bootup_flag = true;
}

static int32_t read_data_to_rx_ringbuffer() {
    char buf[SERIAL_SINGLE_RW_LEN];
    int32_t rd_num, buf_size;

    //buf = mem_alloc(SERIAL_SINGLE_RW_LEN);
    buf_size = rb_available_space(&recv_rb);
    while(buf_size > 0) {
        if(buf_size > SERIAL_SINGLE_RW_LEN)
        buf_size = SERIAL_SINGLE_RW_LEN;

        rd_num = serial_read(handle, buf, buf_size);
#if DUMP_SERIAL_DATA
        if(rd_num > 0)
        dbg_printf_array("[R]:", buf, (uint16_t)rd_num);
#endif
        if(rd_num > 0) {
            dbg_printf("serial_read[%d/%d]\n", rd_num, buf_size);

            if(rb_write(&recv_rb, buf, rd_num) < 0)
                dbg_printf("rb_write recv_rb failed!(%d/%d)\n", rd_num, rb_available_space(&recv_rb));
            if(rd_num < buf_size) { // all data read out
                speedup_read = 0;
                break;
            }
        } else if(rd_num < 0) {
            dbg_printf("serial_read failed in read_data_to_rx_ringbuffer\n");
        }

        buf_size = rb_available_space(&recv_rb);
    }
    //mem_free(buf);

    if(serial_get_rx_count(handle) > 0) {
        speedup_read = 1;
        dbg_printf("WARNING: vfifo has data but ringbuffer full!\n");
    }
    return 0;
}

int32_t as60x_transport_ready_to_read() {
    uint8_t *buf;
    //char packet_head[MSG_HEAD_SIZE+PACKET_HEAD_SIZE]; // 9 bytes
    as60x_recv_packet_common_t *pkt_head;
    uint8_t pkt_head_len = PKT_HEAD_LEN;
    uint8_t temp[PKT_HEAD_LEN];
    int16_t recv_rb_len;

    dbg_printf("transport_ready_to_read suspend:%d\n", transport_suspend);
    if(handle < 0) {
        dbg_printf("transport_ready_to_read: handle invalid!\n");
        return -1;
    }
    if(transport_suspend)
        return -2;

try_more:
    read_data_to_rx_ringbuffer();

    recv_rb_len = rb_remaining_data(&recv_rb);
    
    if(wait_for_bootup_flag) {
        if(recv_rb_len == 1) {
            uint8_t flag;
            rb_read_prepare(&recv_rb, &flag, 1);
            dbg_printf("check bootup flag %x(0x55)!\n", flag);
            //if(flag == 0x55) {
                rb_read_commit(&recv_rb, 1);
                //as60x_bootup_flag_recieved(flag);
                pkt_recv_bootup_flag(flag);
            //}
            wait_for_bootup_flag = 0;
        } else {
            dbg_printf("bootup flag length %d != 1!\n", recv_rb_len);
        }
    }

    // parse and handle ringbuffer data
    while(recv_rb_len > PKT_HEAD_LEN) {
        if(rb_read_prepare(&recv_rb, temp, PKT_HEAD_LEN) == 0) {
            if((*(uint16_t *)temp) == FIXED_PACKET_TOCKEN) {
                // this is to ensure message data aligned
                int32_t pkt_len = (temp[PACKET_LENGTH_OFFSET]<<8) + temp[PACKET_LENGTH_OFFSET+1];
                pkt_len += PKT_HEAD_LEN;
                if(recv_rb_len >= pkt_len) {
                    buf = mem_alloc(pkt_len); // buf should be aligned with 4
                    if(buf != NULL) {
                        //rb_read_commit(&recv_rb, 2); // skip packet token
                        if(rb_read(&recv_rb, buf, pkt_len) == 0) {
                            pkt_head = (as60x_recv_packet_common_t *)(buf+PKT_TOKEN_SIZE);
                            pkt_head->length = word_swap_bytes(pkt_head->length);
                            if(verify_pkt_checksum(pkt_head)) {
                                recv_rb_len -= pkt_len;
                                pkt_recv_handler(pkt_head);
                            } else {
                                dbg_printf("verify_checksum failed!\n");
                            }
                        } else {
                            dbg_printf("read whole packet failed!\n");
                        }
                        mem_free(buf);
                    } else {
                        dbg_printf("mem alloc failed!(%d)\n", __LINE__);
                        break;
                    }
                } else {
                    // msg is not completely received, just wait
                    dbg_printf("not enough msg data, wait.\n");
                    break;
                }
            } else {
                // packet flag mismatch
                dbg_printf("packet token mismatch!(%x-%x)\n", temp[0], temp[1]);
                break;
            }
        } else {
            dbg_printf("rb_read_prepare msg header failed!\n");
            break;
        }

    }

    // read again after ringbuffer data been processed
    if(speedup_read) {
        goto try_more;
    }

    return 0;
}

int32_t as60x_transport_ready_to_write() {
    if(handle < 0)
        return -1;
    if(transport_suspend)
        return -2;
    // check command queue, send if any command in queue.
    as60x_transport_trigger_write();
    return 0;
}

static int32_t push_pkt_to_tx_ringbuffer(as60x_cmd_packet_t *pkt)
{
    int16_t pkt_buf_len = pkt->length + PKT_HEAD_LEN;
    uint16_t token = FIXED_PACKET_TOCKEN;
    uint8_t *ptr = (uint8_t *)pkt;
    uint16_t temp;

    if(rb_available_space(&send_rb) > pkt_buf_len)
    {
        rb_write(&send_rb, (uint8_t *)&token, PKT_TOKEN_SIZE);
        rb_write(&send_rb, ptr, PKT_ADDR_SIZE + PKT_TYPE_SIZE);
        ptr += PKT_ADDR_SIZE + PKT_TYPE_SIZE;

        // big endian for Word length
        temp = word_swap_bytes((*ptr) + (*(ptr+1)<<8));
        //dbg_printf("word_swap_bytes: %x(%x) %x-%x=>%x\n", pkt->length, (*ptr)+(*(ptr+1)<<8), *ptr, *(ptr+1), temp);
        rb_write(&send_rb, (uint8_t *)&temp, PKT_LENGTH_SIZE);
        ptr += PKT_LENGTH_SIZE;

        rb_write(&send_rb, ptr, 1);
        ptr += PKT_COMMAND_SIZE;

        if(pkt->cmd_para.len > 0)
            rb_write(&send_rb, pkt->cmd_para.data, pkt->cmd_para.len);

        temp = pkt_checksum(pkt);
        rb_write(&send_rb, (uint8_t *)&temp, PKT_CHKSUM_SIZE);
        return 0;
    }
    return -1;
}

int32_t as60x_transport_enqueue_pkt(as60x_cmd_packet_t *pkt) {
    int32_t ret;
    //dbg_printf_array("[PKT]:", (uint8_t *)pkt, sizeof(as60x_cmd_packet_t));
    ret = enqueue_tail(&pkt_queue, &pkt);
    if(ret < 0) {
        dbg_printf("as60x_transport_enqueue_pkt failed!\n");
        return ret;
    }

    if(transport_suspend)
        return -2;

    if(handle < 0)
        return -1;

    ret = serial_get_tx_avail(handle);
    //dbg_printf("serial_get_tx_avail: %d\n", ret);
    // generally there is always enough space to write(txvfifo)
    if(ret > 0) {
        as60x_transport_trigger_write();
    } else if(ret < 0) {
        dbg_printf("serial_get_tx_avail failed!\n");
    }

    return ret;
}

static int32_t as60x_transport_trigger_write()
{
    int16_t remain_num;
    int16_t rb_full_flag = false;
    as60x_cmd_packet_t *pkt = get_queue_head(&pkt_queue);

    dbg_printf("as60x_transport_trigger_write\n");
try_more:
    // queue -> ringbuffer
    while(pkt != NULL)
    {
        if(push_pkt_to_tx_ringbuffer(pkt) == 0)
        {
            // successful send to send_buffer
            dequeue_head_pointer(&pkt_queue);
            free_cmd_packet(pkt);
        }
        else
        {
            // insufficient send_rb space
            if(rb_full_flag == true) {
                // ringbuffer still no space
                return 0;
            }
            rb_full_flag = true;
            break;
        }
        pkt = (as60x_cmd_packet_t *)get_queue_head(&pkt_queue);
    }

    // ringbuffer -> serial port
    remain_num = rb_remaining_data(&send_rb);
    while(remain_num > 0)
    {
        int16_t srd_n = (remain_num>SERIAL_SINGLE_RW_LEN?SERIAL_SINGLE_RW_LEN:remain_num);
        char buf[SERIAL_SINGLE_RW_LEN];
        //char *buf = mem_alloc(srd_n);

        dbg_printf("send_rb sending data %d/%d\n", srd_n, remain_num);
        if(rb_read_prepare(&send_rb, buf, srd_n) == 0)
        {
            int32_t num = serial_write(handle, buf, srd_n);
#if DUMP_SERIAL_DATA
            if(num > 0)
                dbg_printf_array("[W]:", buf, (uint16_t)num);
#endif
            //mem_free(buf);
            if(num > 0)
            {
                dbg_printf("serial_write %d/%d\n", num, srd_n);
                if(rb_read_commit(&send_rb, num) < 0){
                    dbg_printf("rb_read_commit failed in as60x_transport_trigger_write!");
                }

                if(num < srd_n) {
                    // write buffer in kernel may be full
                    // should wait for fd notify
                    dbg_printf("partial serial_write\n");
                    break;
                } else {
                    if(rb_full_flag)
                        goto try_more;
                }
            } else {
                // write fail
                dbg_printf("serial_write failed in as60x_transport_trigger_write!\n");
                break;
            }
        }else {
            //mem_free(buf);
        }
        remain_num = rb_remaining_data(&send_rb);
    }

    return 0;
}

int32_t as60x_transport_buff_init()
{
  if(!recv_buff)
    recv_buff = (char *)mem_alloc(RECV_BUFF_SIZE);
  if(!send_buff)
    send_buff = (char *)mem_alloc(SEND_BUFF_SIZE);
  if((!recv_buff) || (!send_buff)) {
    dbg_printf("malloc buff failed!");
    return -1;
  }

  ringbuffer_init(&recv_rb, recv_buff, RECV_BUFF_SIZE);
  ringbuffer_init(&send_rb, send_buff, SEND_BUFF_SIZE);

  queue_init(&pkt_queue, pkt_queue_buffer, TX_PKT_QUEUE_SIZE);

  return 0;
}

int32_t as60x_transport_deinit()
{
  // TODO
  return 0;
}

int32_t as60x_transport_suspend() {
    transport_suspend = 1;
}

int32_t as60x_transport_resume() {
    transport_suspend = 0;
}

int32_t as60x_transport_start(uint16_t port, uint32_t baudrate)
{
  //sprintf(str_fd, "/dev/ttyS%d", port);
  handle = serial_open(port);
  if(handle < 0)
  {
      dbg_printf("open com port error!(%d)", handle);
      return handle;
  }

  serial_setup(handle, baudrate);
  serial_clear_rx(handle);
  //serial_clear_tx(handle);
  //serial_start(handle, transport_ready_to_read, transport_ready_to_write);
  //wait_for_bootup_flag = 1;
  return 0;
}

int32_t as60x_transport_stop()
{
  int32_t ret=0;
  if(handle>0) {
    serial_clear_rx(handle);
    serial_clear_tx(handle);
    ret = serial_close(handle);
    handle = -1;
  }
  return ret;
}

#if 0
int32_t as60x_transport_start_for_download(int16_t port, uint32_t baudrate)
{
  //sprintf(str_fd, "/dev/ttyS%d", port);
  handle = serial_open(port);
  if(handle < 0)
  {
      dbg_printf("open com port error!(%d)", handle);
      return handle;
  }

  serial_setup(handle, baudrate);
  serial_clear_rx(handle);
  
  return 0;
}

int32_t as60x_transport_register_isr_callback_for_download()
{
  //serial_register_isr_callback()
  return 0;
}
#endif

int32_t as60x_transport_write(char *buff, int32_t len, int32_t timeout_ms) {
  //int32_t s_hdl;
  int32_t remain = len;
  uint32_t t1 = os_get_time();

  if(handle<=0)
    return -1;
  
  while(remain > 0) {
    int32_t wr_n = serial_write(handle, buff+(len-remain), remain);
    if(wr_n < 0) {
     /*
      serial_close(s_hdl);
      return -2;
      */
    }
    else if(wr_n > 0) {
      remain -= wr_n;
      dbg_printf("writep:%d/%d\n", wr_n, remain);
    }

#if 0
    if(os_get_duration_ms(t1) > timeout_ms) {
      //serial_close(s_hdl);
      return (len-remain);
    }
    if(wr_n == 0) {
      os_sleep(10);
      serial_notify_write();
    }
#else
        break;
#endif
  }
  
  //serial_close(s_hdl);
  return (len-remain);
}

int32_t as60x_transport_read(char *buff, int32_t len, int32_t timeout_ms) {
  //int32_t s_hdl;
  int32_t remain = len;
  uint32_t t1 = os_get_time();
  uint16_t i;
  //int32_t count;
  
  if(handle<=0)
    return -1;
  
  while(remain > 0) {
    int32_t rd_n=0;
    //if((count=serial_get_rx_count(handle)) > 0) {
      rd_n = serial_read(handle, buff+(len-remain), remain);
      if(rd_n < 0) {
        /*
         serial_close(s_hdl);
         s_hdl = serial_open(port);
         if(s_hdl<=0)
           return -2;
         */
      }
      else if(rd_n > 0) {
        remain -= rd_n;
        dbg_printf("readp:[%d/%d]", rd_n, remain);
        //for(i=rd_n;i>0;i--)
        //  dbg_printf("%x-", buff[len-remain-i]);
        //dbg_printf("\n");
      }
    //}
#if 0
    if(os_get_duration_ms(t1) > timeout_ms) {
      //serial_close(s_hdl);
      return (len-remain);
    }
    if(rd_n == 0)
      os_sleep(10);
#else
    break;
#endif
  }

  //serial_close(s_hdl);
  return (len-remain);
}

int32_t as60x_transport_write_poll(char *buff, int32_t len, int32_t timeout_ms) {
  //int32_t s_hdl;
  int32_t remain = len;
  uint32_t t1 = os_get_time();

  if(handle<=0)
    return -1;
  
  while(remain > 0) {
    int32_t wr_n = serial_write(handle, buff+(len-remain), remain);
    if(wr_n < 0) {
     /*
      serial_close(s_hdl);
      return -2;
      */
    }
    else if(wr_n > 0) {
      remain -= wr_n;
      dbg_printf("writep:%d/%d\n", wr_n, remain);
    }

    if(os_get_duration_ms(t1) > timeout_ms) {
      //serial_close(s_hdl);
      return -3;
    }
    if(wr_n == 0) {
      os_sleep(20);
      serial_notify_write();
    }
  }
  
  //serial_close(s_hdl);
  return 0;
}

int32_t as60x_transport_read_poll(char *buff, int32_t len, int32_t timeout_ms) {
  //int32_t s_hdl;
  int32_t remain = len;
  uint32_t t1 = os_get_time();
  uint16_t i;
  int32_t count;
  
  if(handle<=0)
    return -1;
  
  while(remain > 0) {
    int32_t rd_n=0;
    if((count=serial_get_rx_count(handle)) > 0) {
      rd_n = serial_read(handle, buff+(len-remain), remain);
      if(rd_n < 0) {
        /*
         serial_close(s_hdl);
         s_hdl = serial_open(port);
         if(s_hdl<=0)
           return -2;
         */
      }
      else if(rd_n > 0) {
        remain -= rd_n;
        dbg_printf("readp:[%d/%d]", rd_n, remain);
        for(i=rd_n;i>0;i--)
          dbg_printf("%x-", buff[len-remain-i]);
        dbg_printf("\n");
      }
    }

    if(os_get_duration_ms(t1) > timeout_ms) {
      //serial_close(s_hdl);
      return -3;
    }
    if(count == 0)
      os_sleep(10);
  }

  //serial_close(s_hdl);
  return 0;
}

int32_t as60x_transport_clear_rx(uint16_t port) {
  int32_t s_hdl;
  int32_t ret;
  
  if(handle>0) {
    ret = serial_clear_rx(handle);
  } else {
    s_hdl = serial_open(port);
    ret = serial_clear_rx(s_hdl);
    serial_close(s_hdl);
  }
  return ret;
}


