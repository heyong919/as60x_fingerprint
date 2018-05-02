#ifndef _AS60X_H_
#define _AS60X_H_

#include "stdtypes.h"
#include "transport.h"
#include "operation.h"
#include "hw_config.h"

#define PACKET_TYPE_COMMAND       0x01
#define PACKET_TYPE_RESPONSE      0x07
#define PACKET_TYPE_DATA          0x02
#define PACKET_TYPE_DATA_END      0x08

#define AS60X_BOOTUP_FLAG         0x55

#define IDENTIFY_MATCH_ALL        0xFFFF

#define PKT_TOKEN_SIZE            2
#define PKT_ADDR_SIZE             4
#define PKT_TYPE_SIZE             1
#define PKT_COMMAND_SIZE          1
#define PKT_LENGTH_SIZE           2
#define PKT_CHKSUM_SIZE           2

#define FIXED_PACKET_TOCKEN       0x01ef
#define PACKET_LENGTH_OFFSET      (PKT_TOKEN_SIZE+PKT_ADDR_SIZE+PKT_TYPE_SIZE)
#define PKT_HEAD_LEN              (PACKET_LENGTH_OFFSET+PKT_LENGTH_SIZE)

// command related definitions
//#define AUTO_IDENTIFY_RESULT_

typedef enum {
    RESPONSE_OK =0,
    RESPONSE_PKT_RECV_FAILED,
} AS60X_RESPONSE_CODE;

typedef enum {
    AS60X_STATE_POWER_DOWN,
    AS60X_STATE_POWER_ON,  // power turned on
    AS60X_STATE_WAKEUP_POWER_ON,
    AS60X_STATE_BOOTUP_COMPLETE, // 0x55 flag revieved
    AS60X_STATE_HANDSHAKE_OK, // handshake command ok
    AS60X_STATE_NORMAL,
    AS60X_STATE_SENSOR_SLEEP,
} as60x_module_state_t;

typedef enum {
    RES_FAILED_CANCEL,
    RES_FAILED_RETRY,
    RES_FAILED_OMIT,
    RES_SUCCEED_NEXT,
    RES_SUCCEED_BYPASS
} handle_res_t;


#pragma pack(push, 1)

// handshake
//typedef struct {
//    uint8_t response;
//} res_data_PS_HandShake;

// get SN code
typedef struct {
    uint8_t reserved;
} cmd_para_PS_GetSNCode;

typedef struct {
//    uint8_t response;
    uint8_t sn[32];
} res_data_PS_GetSNCode;

// PS_SetChipAddr
typedef struct {
    uint32_t dev_addr;
} cmd_para_PS_SetChipAddr;

//typedef struct {
//    uint8_t response;
//} res_data_PS_SetChipAddr;

// PS_ReadSysPara
//typedef struct {
//} cmd_para_PS_ReadSysPara;

typedef struct {
//    uint8_t response;
    uint8_t para_table[16];
} res_data_PS_ReadSysPara;

// PS_WriteReg
typedef struct {
    uint8_t reg_id;
    uint8_t data;
} cmd_para_PS_WriteReg;

//typedef struct {
//    uint8_t response;
//} res_data_PS_WriteReg;


// PS_AutoEnroll
typedef struct {
    uint16_t id;
    uint8_t count;
    uint16_t bit_param;
} cmd_para_PS_AutoEnroll;

typedef struct {
//    uint8_t response;
    uint8_t para1;
    uint8_t para2;
} res_data_PS_AutoEnroll;

// PS_AutoIdentify
typedef struct {
    uint8_t sec_level;
    uint16_t id;
    uint16_t bit_param;
} cmd_para_PS_AutoIdentify;

typedef struct {
//    uint8_t response;
    uint8_t para;
    uint16_t id;
    uint16_t score;
} res_data_PS_AutoIdentify;

// PS_DeletChar
typedef struct {
    uint16_t page_id;
    uint16_t count;
} cmd_para_PS_DeletChar;

//typedef struct {
//    uint8_t response;
//} res_data_PS_DeletChar;

// PS_Empty
//typedef struct {
//} cmd_para_PS_Empty;

//typedef struct {
//    uint8_t response;
//} res_data_PS_Empty;

// PS_Cancel
//typedef struct {
//} cmd_para_PS_Cancel;

//typedef struct {
//    uint8_t response;
//} res_data_PS_Cancel;

// PS_Sleep
//typedef struct {
//} cmd_para_PS_Sleep;

//typedef struct {
//    uint8_t response;
//} res_data_PS_Sleep;

// PS_ValidTempleteNum
//typedef struct {
//} cmd_para_PS_ValidTempleteNum;

typedef struct {
//    uint8_t response;
    uint16_t valid_count;
} res_data_PS_ValidTempleteNum;

// PS_SetPwd
typedef struct {
    uint32_t passwd;
} cmd_para_PS_SetPwd;

//typedef struct {
//    uint8_t response;
//} res_data_PS_SetPwd;

// PS_VfyPwd
typedef struct {
    uint32_t passwd;
} cmd_para_PS_VfyPwd;

//typedef struct {
//    uint8_t response;
//} res_data_PS_VfyPwd;

#pragma pack(pop)

typedef struct {
    as60x_module_state_t state;
    as60x_operation_t *current_op;
    op_para_t identify_op_para;
    uint32_t dev_addr;
} as60x_context_t;

struct as60x_config {
    uint32_t passwd;
    uint32_t dev_addr;
} as60x_config_t;

typedef struct {
    uint8_t repeat_num;
    uint8_t led_mode;
    uint8_t pre_proc;
    uint8_t allow_override;
    uint8_t disallow_dup_finger;
    uint8_t continuous_capture;
    uint16_t store_pos;
} register_finger_config_t;

static void as60x_sensor_eint_hisr();

#endif

