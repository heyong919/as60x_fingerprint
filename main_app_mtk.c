/*
 ============================================================================
 Name        : da14580_host_app.c
 Author      : hey
 Version     :
 Copyright   : Your copyright notice
 ============================================================================
 */
#ifdef __BLE_TASK_SUPPORT__
#ifdef __DA14580_BLE_SUPPORT__
#ifdef APP_ON_MTK_NUCLEUS

#include "co_error.h"
#include "app_msg.h"
#include "app_api.h"
#include "profiles.h"
#include "transport.h"
#include "task_config.h"
#include "syscomp_config.h"
#include "dcl_uart.h"
#include "ble_msg_struct.h"
#include "binary.h"

#define DL_RETRY_RD  1
#define DL_RETRY_WR  2
#define DL_SUCCEED  0
#define DOWNLOAD_STATE_INITIAL 0x00
#define DOWNLOAD_STATE_DONE 0x40
#define MAX_DOWNLOAD_RETRY  3

#define DA14580_UART_PORT  0 //uart_port1
#define DA14580_UART_DOWNLOAD_BAUDRATE  57600
#define DA14580_UART_GTL_BAUDRATE  115200


//extern unsigned char raw_code[];
extern const char gpio_ext_ble_rst_pin;

static int8_t download_steps=DOWNLOAD_STATE_INITIAL;
static int8_t download_waiting_for;
static int8_t da14580_init_done=0;

extern int32_t da14580_init_stack_handlers(int16_t port, uint32_t baudrate);
extern int32_t da14580_start_operations();
extern int32_t user_set_dev_config_perpheral();
extern int32_t user_set_dev_config_central();


static void ble_chip_reset();
static char calc_code_crc(char *code_buff, uint32_t len);
static int32_t ble_firmware_download();
static int32_t ble_power_on_init();

static int32_t da14580_boot_code(int16_t port, uint32_t baudrate, char *code, uint32_t len, char crc);


static void ble_chip_reset()
{
  dbg_printf("ble_chip_reset\n");
  //GPIO_ModeSetup(gpio_ext_ble_rst_pin, GPIO_MODE0);
  GPIO_WriteIO(1, gpio_ext_ble_rst_pin);
  kal_sleep_task(KAL_TICKS_10_MSEC);
  GPIO_WriteIO(0, gpio_ext_ble_rst_pin);
  kal_sleep_task(KAL_TICKS_10_MSEC);
}

static char calc_code_crc(char *code_buff, uint32_t len)
{
  char res=0;
  char *temp = code_buff;
  uint32_t i;

  for(i=0;i<len;i++) {
    res = (res^(*temp));
    temp++;
  }
  return res;
}

static int32_t ble_firmware_download(char *code, uint32_t len, char crc)
{
  int32_t ret;
  ret = da14580_boot_code(DA14580_UART_PORT, DA14580_UART_DOWNLOAD_BAUDRATE, code, len, crc);
  if(ret<0) {
    dbg_printf("da14580_boot_code failed!(%d)\n", ret);
    download_steps=0;
    download_waiting_for=0;
    return ret;
  }
  else if(ret==DL_RETRY_RD || ret==DL_RETRY_WR) {
    download_waiting_for = ret;
    dbg_printf("da14580_boot_code step(%d) RETRY!(%d)\n", download_steps, ret);
    return ret;
  }

  // download succeed
  return 0;
}

static int32_t ble_power_on_init()
{
  int32_t ret;
  ret = da14580_init_stack_handlers(DA14580_UART_PORT, DA14580_UART_GTL_BAUDRATE);
  if(ret<0) {
    dbg_printf("da14580_init_stack_handlers FAILED!\n");
    return ret;
  }
  da14580_init_done = 1;
  da14580_start_operations();
  return ret;
}

#define UART_GET_STX_LEN 32
static int32_t da14580_boot_code(int16_t port, uint32_t baudrate, char *code, uint32_t len, char crc) {
  static char rd_buf[32];
  static char wr_buf[4];
  static char *bin_code;
  static char bin_crc;
  static uint32_t bin_len;
  static uint16_t pos=0, len_to_write;
  static int8_t retry;
  int32_t ret;

  dbg_printf("da14580_boot_code step(%d)\n", download_steps);

  switch(download_steps) {
    case 0:
    {
      bin_code = code;
      bin_crc = crc;
      bin_len = len;
      retry = 10; // retry for STX
      download_steps++;
    }
    case 1:
    {
      if((ret=transport_start_for_download(port, baudrate)) < 0) {
        dbg_printf("transport_start_for_download failed!(%d)\n", ret);
        return -download_steps;
      }
      download_steps++;
      // wait for data
      return DL_RETRY_RD;
    }

    case 2:
    {
      //int8_t loop;
      do {
        retry--;
        if((ret=transport_read(rd_buf, UART_GET_STX_LEN, 100)) < 0) {
          dbg_printf("read STX(0x02) failed!(%d)(%d)\n", ret, retry);
          return -download_steps;
        }
        else if(ret == 0) {
          return DL_RETRY_RD;
        }
        else if(ret < UART_GET_STX_LEN) { // all data read out
          dbg_printf("read out last data(%d)(%d)\n", ret, rd_buf[ret-1]);
          if(rd_buf[ret-1] == 0x02)
            break;
          else
            return DL_RETRY_RD;
        }
        else if(ret == UART_GET_STX_LEN) {
          // should read more data
        }
      } while(retry>0);
      
      if(retry < 0) {
        dbg_printf("STX(%x) != 0x02 failed!(%d)\n", rd_buf[0], retry);
        return -download_steps;
      }
      download_steps++;
      
      wr_buf[0] = 0x01;
      wr_buf[1] = (bin_len&0xFF);
      wr_buf[2] = (bin_len>>8)&0xFF;
      pos = 0;
      len_to_write = 3;
    }

    case 3:
    {
      if((ret=transport_write(wr_buf+pos, len_to_write, 100)) < 0) {
        dbg_printf("send code length failed!(%d)\n", ret);
        return -download_steps;
      }
      else if(ret<len_to_write) {
        len_to_write -= ret;
        pos += ret;
        return DL_RETRY_WR;
      }
      download_steps++;
    }

    case 4:
    {
      if((ret=transport_read(rd_buf, 1, 100)) < 0) {
        dbg_printf("read ACK failed!(%d)\n", ret);
        return -download_steps;
      }
      else if(ret<1) {
        return DL_RETRY_RD;
      }
      
      if(rd_buf[0] != 0x06/*ACK*/) {
        dbg_printf("ACK(%x) != 0x06 failed!\n", rd_buf[0]);
        return -download_steps;
      }
      download_steps++;

      pos = 0;
      len_to_write = bin_len;
    }

    case 5:
    {
      if((ret=transport_write(bin_code+pos, len_to_write, 200)) < 0) {
        dbg_printf("send code data failed!(%d)\n", ret);
        return -download_steps;
      }
      else if(ret<len_to_write) {
        len_to_write -= ret;
        pos += ret;
        return DL_RETRY_WR;
      }
      download_steps++;
    }

    case 6:
    {
      if((ret=transport_read(rd_buf, 1, 100)) < 0) {
        dbg_printf("read code CRC failed!(%d)\n", ret);
        return -download_steps;
      }
      else if(ret<1) {
        return DL_RETRY_RD;
      }
      
      if(rd_buf[0] != bin_crc) {
        dbg_printf("CRC(%x) != %x failed!\n", rd_buf[0], bin_crc);
        return -download_steps;
      }
      download_steps++;
    }

    case 7:
    {
      wr_buf[0] = 0x06; /*ACK*/
      if((ret=transport_write(wr_buf, 1, 100)) < 0) {
        dbg_printf("send ACK(0x06) failed!(%d)\n", ret);
        return -download_steps;
      }
      else if(ret<1) {
        return DL_RETRY_WR;
      }
      download_steps++;
    }

    case 8:
    {
      int32_t ret = transport_stop();
      if(ret < 0)
        dbg_printf("transport_stop failed!(%d)\n", ret);
      download_waiting_for = 0;
      download_steps = DOWNLOAD_STATE_DONE;
    }
    
  }
  
  dbg_printf("code start running...\n");
  return DL_SUCCEED;
}

static void ble_send_ilm_int(msg_type msgid) {
  ilm_struct *ilm;

  ilm = allocate_ilm(MOD_BLE);
  ilm->msg_id = msgid;
  SEND_ILM(MOD_BLE, MOD_BLE, BT_APP_SAP, ilm);
}

static void retry_download() {
  static kal_uint8 retry_download_count=0;
  ilm_struct *ilm;

  if(retry_download_count < MAX_DOWNLOAD_RETRY) {
    retry_download_count++;
    ble_send_ilm_int(MSG_ID_BLE_INIT_DOWNLOAD);
    //ilm = allocate_ilm(MOD_BLE);
    //ilm->msg_id = MSG_ID_BLE_INIT_DOWNLOAD;
    //SEND_ILM(MOD_BLE, MOD_BLE, BT_APP_SAP, ilm);
  }
}


#if defined(__MTK_TARGET__) && defined(__DCM_WITH_COMPRESSION_MAUI_INIT__)
#pragma push
#pragma arm section code="DYNAMIC_COMP_MAUIINIT_SECTION"
#endif

kal_bool ble_task_init(task_indx_type task_indx)
{
  /* Do task's initialization here.
   * Notice that: shouldn't execute modules reset handler since 
   * stack_task_reset() will do. */
  return KAL_TRUE;
}

#if defined(__MTK_TARGET__) && defined(__DCM_WITH_COMPRESSION_MAUI_INIT__)
#pragma arm section code
#pragma pop
#endif

kal_bool ble_task_reset(task_indx_type task_indx)
{
  /* Do task's reset here.
   * Notice that: shouldn't execute modules reset handler since 
   * stack_task_reset() will do. */
  return KAL_TRUE;
}

kal_bool ble_task_end(task_indx_type task_indx)
{
  /* Do task's termination here.
   * Notice that: shouldn't execute modules reset handler since 
   * stack_task_end() will do. */
  return KAL_TRUE;
}

void ble_task_main(task_entry_struct *task_entry_ptr)
{
  ilm_struct current_ilm;
  kal_uint32 my_index;     

  kal_get_my_task_index(&my_index);     
  stack_set_active_module_id(my_index, MOD_BLE);
  while(1)
  {
    receive_msg_ext_q_for_stack(task_info_g[task_entry_ptr->task_indx].task_ext_qid, &current_ilm);
    
    switch(current_ilm.msg_id)
    {
      case MSG_ID_UART_READY_TO_READ_IND:
      if(download_waiting_for == DL_RETRY_RD) {
        if(ble_firmware_download(NULL, 0, 0) < 0) {
          retry_download();
        }
      }
      if(da14580_init_done)
        transport_ready_to_read();
      break;
      
      case MSG_ID_UART_READY_TO_WRITE_IND:
      if(download_waiting_for == DL_RETRY_WR) {
        if(ble_firmware_download(NULL, 0, 0) < 0) {
          retry_download();
        }
      }
      if(da14580_init_done)
        transport_ready_to_write();
      break;

      case MSG_ID_BLE_INIT_DOWNLOAD:
      {
        char crc = calc_code_crc(raw_code, sizeof(raw_code));
        download_steps=DOWNLOAD_STATE_INITIAL;
        download_waiting_for=0;
    
        ble_chip_reset();
        dbg_printf("ble_firmware_download crc:%x", crc);
        if(ble_firmware_download(raw_code, sizeof(raw_code), crc) < 0) {
          retry_download();
        }
      }
      break;

      case MSG_ID_BLE_POWER_ON_INIT:
      {
        ble_power_on_init();
      }
      break;

      case MSG_ID_BLE_RESET_REQ:
        ble_chip_reset();
        ble_send_ilm_int(MSG_ID_BLE_INIT_DOWNLOAD);
      break;
      
      case MSG_ID_BLE_GAP_SET_ROLE:
      {
        user_operation_t set_role_ops;
        ble_set_role_t *set_role = (ble_set_role_t *)(current_ilm.local_para_ptr);
        
        set_role_ops.event_id = GAPM_CMP_EVT;
        set_role_ops.handler = NULL;
        if(set_role->role == GAP_ROLE_PERIPHERAL) {
          set_role_ops.command = user_set_dev_config_perpheral;
        } else {
          set_role_ops.command = user_set_dev_config_central;
        }
        app_add_single_operation(&set_role_ops);
      }
      break;

      case MSG_ID_BLE_GAP_ADVERTISE:
      break;

      case MSG_ID_BLE_GAP_CONNECT_CFM:
      break;

      case MSG_ID_BLE_GAP_DISCONNECT:
      break;

      // GATT server
      case MSG_ID_BLE_GATTS_ADD_SERVICE:
      break;

      case MSG_ID_BLE_GATTS_ADD_CHARACTERISTIC:
      break;

      case MSG_ID_BLE_GATTS_ADD_DESCRIPTOR:
      break;

      case MSG_ID_BLE_GATTS_SVC_GET_PERMISSION:
      break;

      case MSG_ID_BLE_GATTS_SVC_SET_PERMISSION:
      break;

      case MSG_ID_BLE_GATTS_SEND_INDICATION:
      break;

      case MSG_ID_BLE_GATTS_CHAR_WRITE_CFM:
      break;


/*
MSG_ID_BLE_GAP_SET_ROLE,
MSG_ID_BLE_GAP_ADVERTISE,
MSG_ID_BLE_GAP_CONNECT_IND,
MSG_ID_BLE_GAP_CONNECT_CFM,
MSG_ID_BLE_GAP_DISCONNECT,
MSG_ID_BLE_GAP_DISCONNECT_IND,

MSG_ID_BLE_GATTS_ADD_SERVICE,
MSG_ID_BLE_GATTS_ADD_CHARACTERISTIC,
MSG_ID_BLE_GATTS_ADD_DESCRIPTOR,
MSG_ID_BLE_GATTS_SVC_GET_PERMISSION,
MSG_ID_BLE_GATTS_SVC_SET_PERMISSION,
MSG_ID_BLE_GATTS_SEND_INDICATION,
MSG_ID_BLE_GATTS_CHAR_WRITE_IND,
MSG_ID_BLE_GATTS_CHAR_WRITE_CFM,
MSG_ID_BLE_GATTS_CHAR_READ_IND,
*/
      default:
      break;
    }
    free_ilm(&current_ilm);
  }

}

kal_bool ble_create(comptask_handler_struct **handle)
{
  static const comptask_handler_struct ble_handler_info = 
  {
    ble_task_main,      /* task entry function */
    ble_task_init,      /* task initialization function */
    NULL,   /* task configuration function */
    ble_task_reset,   /* task reset handler */
    ble_task_end      /* task termination handler */
  };
  
  *handle = (comptask_handler_struct *)&ble_handler_info;
  return KAL_TRUE;
}

#endif
#endif
#endif
