#include "kal_general_types.h"
#include "kal_public_defs.h"
#include "kal_public_api.h"
#include "drv_features.h"
#include "dcl.h"

#include "stdtypes.h"
#include "serial.h"

#include "uart_internal.h"

static uint8_t write_available=1;
static DCL_HANDLE s_handle=-1;

static ready_to_read_callback_t ready_to_read_notify=NULL;
static ready_to_write_callback_t ready_to_write_notify=NULL;


int32_t serial_setup(int32_t handle, uint32_t baudrate)
{
  UART_CTRL_DCB_T data;
  
  data.u4OwenrId = MOD_FP; 
  data.rUARTConfig.u4Baud = baudrate;
  data.rUARTConfig.u1DataBits = LEN_8;
  data.rUARTConfig.u1StopBits = SB_1;
  data.rUARTConfig.u1Parity = PA_NONE;
  data.rUARTConfig.u1FlowControl = FC_NONE;
  data.rUARTConfig.ucXonChar = 0x11;
  data.rUARTConfig.ucXoffChar = 0x13;
  data.rUARTConfig.fgDSRCheck = (DCL_BOOLEAN)KAL_FALSE;
  
  DclSerialPort_Control(handle, SIO_CMD_SET_DCB_CONFIG, (DCL_CTRL_DATA_T*)&data);

  return 0;
}

int32_t serial_start(int32_t handle, ready_to_read_callback_t read_cb, ready_to_write_callback_t write_cb)
{
  // already started when open, read/write indication will be received in task ext queue
  // so donot register callback from lower layer
  //ready_to_read_notify = read_cb;
  //ready_to_write_notify = write_cb;
  return 0;
}

// these two functions are unused on MAUI platform
int32_t serial_notify_read() {
  if(s_handle>0 && ready_to_read_notify)
    ready_to_read_notify();
  return 0;
}

int32_t serial_notify_write() {
  write_available =1;
  if(s_handle>0 && ready_to_write_notify)
    ready_to_write_notify();
  return 0;
}

int32_t serial_write(int32_t handle, char *buff, uint32_t length)
{
  int32_t ret;

//  if(write_available) {
    UART_CTRL_PUT_BYTES_T data;
    data.u2Length = (uint16_t)(length&0xFFFF);
    data.u4OwenrId = MOD_FP;
    data.puBuffaddr = buff;
    
    ret = DclSerialPort_Control(handle, SIO_CMD_PUT_BYTES, (DCL_CTRL_DATA_T*) &data);
    //ret = data.u2RetSize;
//  }
//  else
//    ret = 0;
  
  if (ret < 0) {
    dbg_printf("serial_write failed(%d)\r\n", ret);
    return ret;
  }
  else if(ret < length) {
    // should add fd to monitor and wait for notify
//    write_available = 0;
  }
  return data.u2RetSize;
}

int32_t serial_read(int32_t handle, char *buff, uint32_t len)
{
  int32_t ret;
  uint8_t status;
  UART_CTRL_GET_BYTES_T data;

  data.u4OwenrId = MOD_FP;
  data.u2Length = (uint16_t)(len&0xFFFF);
  data.puBuffaddr = buff;
  data.pustatus = &status;
  ret = DclSerialPort_Control(handle, SIO_CMD_GET_BYTES, (DCL_CTRL_DATA_T*)&data);
  //ret = data.u2RetSize; 
  if (ret < 0) {
    dbg_printf("serial_read failed(%d)\r\n", ret);
    return ret;
  }
  return data.u2RetSize;
}

int32_t serial_get_rx_count(int32_t handle)
{
  int32_t ret;
  UART_CTRL_RX_AVAIL_T data;

  ret = DclSerialPort_Control(handle, SIO_CMD_GET_RX_AVAIL, (DCL_CTRL_DATA_T*)&data);
  if(ret == STATUS_OK)
    return data.u2RetSize;
  else
    return ret;
}

int32_t serial_get_tx_avail(int32_t handle)
{
  int32_t ret;
  UART_CTRL_TX_AVAIL_T data;

  ret = DclSerialPort_Control(handle, SIO_CMD_GET_TX_AVAIL, (DCL_CTRL_DATA_T*)&data);
  if(ret == STATUS_OK)
    return data.u2RetSize;
  else
    return ret;
}

int32_t serial_clear_rx(int32_t handle)
{
  int32_t ret;
  //UART_CTRL_PURGE_T data;
  UART_CTRL_CLR_BUFFER_T clr_buf_data;

  //data.u4OwenrId = MOD_FP;
  //data.dir = DCL_RX_BUF;
  //ret = DclSerialPort_Control(handle, SIO_CMD_PURGE, (DCL_CTRL_DATA_T*)&data);

  clr_buf_data.u4OwenrId = MOD_FP;
  ret |= DclSerialPort_Control(handle, SIO_CMD_CLR_RX_BUF, (DCL_CTRL_DATA_T*)&clr_buf_data);
  
  return ret;
}

int32_t serial_clear_tx(int32_t handle)
{
  int32_t ret;
  //UART_CTRL_PURGE_T data;
  UART_CTRL_CLR_BUFFER_T clr_buf_data;

  //data.u4OwenrId = MOD_FP;
  //data.dir = DCL_TX_BUF;
  //ret = DclSerialPort_Control(handle, SIO_CMD_PURGE, (DCL_CTRL_DATA_T*)&data);

  clr_buf_data.u4OwenrId = MOD_FP;
  ret |= DclSerialPort_Control(handle, SIO_CMD_CLR_TX_BUF, (DCL_CTRL_DATA_T*)&clr_buf_data);
  
  return ret;
}

int32_t serial_open(uint16_t port)
{
  int32_t hdl;

  // debug
  //extern UARTStruct UARTPort[];
  //dbg_printf("UART1 owner: %d(UART1_HISR=%d)\n", UARTPort[uart_port1].ownerid, MOD_UART1_HISR);
  // end

  hdl = DclSerPort_Open(port, MOD_FP);
  dbg_printf("serial_open port:%d, handle:%x\n", port, hdl);
  if (hdl > 0) {
    s_handle = hdl;
  } else {
    dbg_printf("Unable to open serial port: %d\r\n", port);
  }
  return hdl;
}

int32_t serial_close(int32_t handle)
{
  int32_t ret;
  ret = DclSerPort_Close(handle);
  s_handle = -1;
  return ret;
}
