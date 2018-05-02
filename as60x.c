/*
 ============================================================================
 Name        : as60x.c
 Author      : hey
 Version     :
 Copyright   : Your copyright notice
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include "dcl.h"
#include "dcl_gpio.h"
#include "as60x.h"
#include "locker.h"


//#define AS60X_UART_DEFAULT_BAUDRATE    57600
#define AS60X_ADDR    0x1808 // current not used

as60x_context_t as60x_cntx = {0};

kal_uint8 touch_out_state = 0;

struct as60x_config as60x_init_config = {
    0,
    0xFFFFFFFF
};

extern void fp_init_done_ind(bool res);

//test only
static uint8_t test_lock=DIR_LOCK;

static void as60x_register_touch_out_int() {
    uint8_t pol;
    pol = GPIO_ReadIO(GPIO_FP_TOUCH_OUT_NUM)?0:1;

    dbg_printf("as60x_register_touch_out_int: Polarity(%d)\n", pol);
    // sensor int
    //EINT_Registration(GPIO_FP_SENSOR_INT_NUM, KAL_TRUE, pol, as60x_sensor_eint_hisr, KAL_TRUE);
    // donot unmask after reg
    EINT_Registration_Ext(GPIO_FP_SENSOR_INT_NUM, KAL_TRUE, pol, as60x_sensor_eint_hisr, KAL_TRUE);
    EINT_Mask(GPIO_FP_SENSOR_INT_NUM);
    EINT_Set_Sensitivity(GPIO_FP_SENSOR_INT_NUM, 0/*LEVEL_SENSITIVE*/);
    //EINT_Set_HW_Debounce(GPIO_FP_SENSOR_INT_NUM, 1);
}

static void as60x_module_power_on() {
    DCL_HANDLE gpio_hnd;

    // set 3V3 LDO power on
    gpio_hnd = DclGPIO_Open(DCL_GPIO, GPIO_FP_3V3_POWER_EN);
    DclGPIO_Control(gpio_hnd, GPIO_CMD_DISABLE_PULL, NULL);
    DclGPIO_Control(gpio_hnd, GPIO_CMD_SET_DIR_OUT, NULL);
    DclGPIO_Control(gpio_hnd, GPIO_CMD_SET_MODE_0, NULL);
    DclGPIO_Control(gpio_hnd, GPIO_CMD_WRITE_HIGH, NULL);
    DclGPIO_Close(gpio_hnd);

    // set uart pin low
    gpio_hnd = DclGPIO_Open(DCL_GPIO, GPIO_FP_UTXD_PIN);
    DclGPIO_Control(gpio_hnd, GPIO_CMD_DISABLE_PULL, NULL);
    DclGPIO_Control(gpio_hnd, GPIO_CMD_SET_DIR_OUT, NULL);
    DclGPIO_Control(gpio_hnd, GPIO_CMD_SET_MODE_0, NULL);
    DclGPIO_Control(gpio_hnd, GPIO_CMD_WRITE_LOW, NULL);
    // test
    /*DclGPIO_Control(gpio_hnd, GPIO_CMD_WRITE_HIGH, NULL);
    DclGPIO_Control(gpio_hnd, GPIO_CMD_WRITE_LOW, NULL);
    DclGPIO_Control(gpio_hnd, GPIO_CMD_WRITE_HIGH, NULL);
    DclGPIO_Control(gpio_hnd, GPIO_CMD_WRITE_LOW, NULL);
    DclGPIO_Control(gpio_hnd, GPIO_CMD_WRITE_HIGH, NULL);
    DclGPIO_Control(gpio_hnd, GPIO_CMD_WRITE_LOW, NULL);*/
    //
    DclGPIO_Close(gpio_hnd);
    gpio_hnd = DclGPIO_Open(DCL_GPIO, GPIO_FP_URXD_PIN);
    DclGPIO_Control(gpio_hnd, GPIO_CMD_DISABLE_PULL, NULL);
    DclGPIO_Control(gpio_hnd, GPIO_CMD_SET_DIR_OUT, NULL);
    DclGPIO_Control(gpio_hnd, GPIO_CMD_SET_MODE_0, NULL);
    DclGPIO_Control(gpio_hnd, GPIO_CMD_WRITE_LOW, NULL);
    DclGPIO_Close(gpio_hnd);

    kal_sleep_task(1); // 4.615ms

    // mcu power
    gpio_hnd = DclGPIO_Open(DCL_GPIO, GPIO_FP_AS60X_POWER_EN);
    DclGPIO_Control(gpio_hnd, GPIO_CMD_DISABLE_PULL, NULL);
    DclGPIO_Control(gpio_hnd, GPIO_CMD_SET_DIR_OUT, NULL);
    DclGPIO_Control(gpio_hnd, GPIO_CMD_SET_MODE_0, NULL);
    DclGPIO_Control(gpio_hnd, GPIO_CMD_WRITE_HIGH, NULL);
    DclGPIO_Close(gpio_hnd);

    kal_sleep_task(1); // 4.615ms

    // set uart mode
    gpio_hnd = DclGPIO_Open(DCL_GPIO, GPIO_FP_UTXD_PIN);
    DclGPIO_Control(gpio_hnd, GPIO_CMD_SET_MODE_1, NULL);
    DclGPIO_Close(gpio_hnd);
    gpio_hnd = DclGPIO_Open(DCL_GPIO, GPIO_FP_URXD_PIN);
    DclGPIO_Control(gpio_hnd, GPIO_CMD_SET_MODE_1, NULL);
    DclGPIO_Close(gpio_hnd);

}

static void as60x_module_power_off() {
    DCL_HANDLE gpio_hnd;

    // set uart pin low
    gpio_hnd = DclGPIO_Open(DCL_GPIO, GPIO_FP_UTXD_PIN);
    DclGPIO_Control(gpio_hnd, GPIO_CMD_DISABLE_PULL, NULL);
    DclGPIO_Control(gpio_hnd, GPIO_CMD_SET_DIR_OUT, NULL);
    DclGPIO_Control(gpio_hnd, GPIO_CMD_SET_MODE_0, NULL);
    DclGPIO_Control(gpio_hnd, GPIO_CMD_WRITE_LOW, NULL);
    DclGPIO_Close(gpio_hnd);

    gpio_hnd = DclGPIO_Open(DCL_GPIO, GPIO_FP_URXD_PIN);
    DclGPIO_Control(gpio_hnd, GPIO_CMD_DISABLE_PULL, NULL);
    DclGPIO_Control(gpio_hnd, GPIO_CMD_SET_DIR_OUT, NULL);
    DclGPIO_Control(gpio_hnd, GPIO_CMD_SET_MODE_0, NULL);
    DclGPIO_Control(gpio_hnd, GPIO_CMD_WRITE_LOW, NULL);
    DclGPIO_Close(gpio_hnd);

    kal_sleep_task(1); // 4.615ms

    // mcu power
    gpio_hnd = DclGPIO_Open(DCL_GPIO, GPIO_FP_AS60X_POWER_EN);
    DclGPIO_Control(gpio_hnd, GPIO_CMD_DISABLE_PULL, NULL);
    DclGPIO_Control(gpio_hnd, GPIO_CMD_SET_DIR_OUT, NULL);
    DclGPIO_Control(gpio_hnd, GPIO_CMD_SET_MODE_0, NULL);
    DclGPIO_Control(gpio_hnd, GPIO_CMD_WRITE_LOW, NULL);
    DclGPIO_Close(gpio_hnd);
}

static void as60x_module_shutdown_for_sleep() {
    dbg_printf("as60x_module_shutdown_for_sleep\n");

    as60x_module_power_off();
    as60x_transport_suspend();
    EINT_UnMask(GPIO_FP_SENSOR_INT_NUM);
    //as60x_transport_wait_bootflag_on();
}

static void as60x_module_bootup_resume() {
    dbg_printf("as60x_module_bootup_resume\n");

    EINT_Mask(GPIO_FP_SENSOR_INT_NUM);
    as60x_module_power_on();
    as60x_transport_resume();
    as60x_transport_wait_bootflag_on();
    
    as60x_cntx.state = AS60X_STATE_WAKEUP_POWER_ON;
}

static void as60x_sensor_power_on() {
    DCL_HANDLE gpio_hnd;

    // set 3V3 LDO power on
    gpio_hnd = DclGPIO_Open(DCL_GPIO, GPIO_FP_3V3_POWER_EN);
    DclGPIO_Control(gpio_hnd, GPIO_CMD_DISABLE_PULL, NULL);
    DclGPIO_Control(gpio_hnd, GPIO_CMD_SET_DIR_OUT, NULL);
    DclGPIO_Control(gpio_hnd, GPIO_CMD_SET_MODE_0, NULL);
    DclGPIO_Control(gpio_hnd, GPIO_CMD_WRITE_HIGH, NULL);
    DclGPIO_Close(gpio_hnd);

    // sensor power
    gpio_hnd = DclGPIO_Open(DCL_GPIO, GPIO_FP_SENSOR_POWER_EN);
    DclGPIO_Control(gpio_hnd, GPIO_CMD_DISABLE_PULL, NULL);
    DclGPIO_Control(gpio_hnd, GPIO_CMD_SET_DIR_OUT, NULL);
    DclGPIO_Control(gpio_hnd, GPIO_CMD_SET_MODE_0, NULL);
    DclGPIO_Control(gpio_hnd, GPIO_CMD_WRITE_HIGH, NULL);
    DclGPIO_Close(gpio_hnd);

}

static void as60x_sensor_power_off() {
    DCL_HANDLE gpio_hnd;
    // sensor power
    gpio_hnd = DclGPIO_Open(DCL_GPIO, GPIO_FP_SENSOR_POWER_EN);
    DclGPIO_Control(gpio_hnd, GPIO_CMD_DISABLE_PULL, NULL);
    DclGPIO_Control(gpio_hnd, GPIO_CMD_SET_DIR_OUT, NULL);
    DclGPIO_Control(gpio_hnd, GPIO_CMD_SET_MODE_0, NULL);
    DclGPIO_Control(gpio_hnd, GPIO_CMD_WRITE_LOW, NULL);
    DclGPIO_Close(gpio_hnd);
}

static void as60x_handshake_result_cb(as60x_operation_t *op, bool result) {
    if(result) {
        //if(as60x_cntx.state == AS60X_STATE_BOOTUP_COMPLETE)
            //as60x_cntx.state = AS60X_STATE_HANDSHAKE_OK;
    }
    //destroy_operation(op);
}

static int32_t as60x_handshake_coroutine(ccrContParam, as60x_operation_t *op) {
    ccrBeginContext;
    //int i;
    ccrEndContext(p);

    ccrBegin(p);
    as60x_ps_handshake();
    ccrReturn(AS60X_OPERATION_DEAL_PARTIAL);
    WAIT_FOR_RESPONSE(op->res_evt);
    if(op->res_evt->u.res_packet.response != RESPONSE_OK) {
        dbg_printf("as60x_handshake_coroutine failed!\n");
        ccrReturn(AS60X_OPERATION_FAILED);
    }
    // no need to fill out_para
    
    ccrFinish(AS60X_OPERATION_COMPLETED);
}

static void as60x_wakeup_identify_result_cb(as60x_operation_t *op, bool result) {
    if(result) {
        uint16_t *para = (uint16_t *)op->para.out_para;
        uint16_t stage = para[0];
        uint16_t code = para[1];
        if(stage == 5 && code == 0) {
            // todo
            dbg_printf("identify result OK!(pos:%d score:%d)\n", para[0], para[1]);
        } else {
            dbg_printf("identify result FAILED!(stage:%d code:%d)\n", stage, code);
        }
    } else {
        dbg_printf("identify operation failed!\n");
    }

    // test: open locker
    lock_unlock(DIR_UNLOCK);
    test_lock = !test_lock;

    //destroy_operation(op);
}

static int32_t as60x_wakeup_identify_coroutine(ccrContParam, as60x_operation_t *op) {
    ccrBeginContext;
    //int result;
    ccrEndContext(p);
    uint8_t *para;
    uint16_t pos, score;
    as60x_res_packet_t *pkt;
    uint16_t *out_para = (uint16_t *)op->para.out_para;

    ccrBegin(p);
    memset(out_para, 0, 8);

    as60x_module_bootup_resume();
    ccrReturn(AS60X_OPERATION_DEAL_PARTIAL);
    WAIT_FOR_BOOTFLAG(op->boot_flag);
    if(op->boot_flag != AS60X_BOOTUP_FLAG) {
        dbg_printf("bootup flag mismatch!(%x)\n", op->boot_flag);
        fp_module_reset();
        ccrReturn(AS60X_OPERATION_FAILED);
    }
    as60x_ps_auto_identify(AS60X_SECURITY_LEVEL, IDENTIFY_MATCH_ALL, true, true);
    ccrReturn(AS60X_OPERATION_DEAL_PARTIAL);
    while(1) {
        WAIT_FOR_RESPONSE(op->res_evt);

        if(op->res_evt->u.res_packet.res_para.len != 5) {
            dbg_printf("identify: recv packet para lens mismatch failed!(%d)\n",
                        op->res_evt->u.res_packet.res_para.len);
            ccrReturn(AS60X_OPERATION_FAILED);
        }

        pkt = &(op->res_evt->u.res_packet);
        para = pkt->res_para.data;
        pos = (*(para+1))<<8 | (*(para+2));
        score = (*(para+3))<<8 | (*(para+4));
        if(pkt->response != RESPONSE_OK) {
            dbg_printf("identify: response failed!(%d, %d)\n", pkt->response, *para);
            out_para[0] = *para; // fail stage
            out_para[1] = pkt->response; // err code
            //ccrReturn(AS60X_OPERATION_FAILED);
            break;
        } else {
            if(*para == 0x00) {
                dbg_printf("identify: command validation check PASS\n");
                ccrReturn(AS60X_OPERATION_DEAL_PARTIAL);
            } else if(*para == 0x01) {
                dbg_printf("identify: geting image PASS\n");
                ccrReturn(AS60X_OPERATION_DEAL_PARTIAL);
            } else if(*para == 0x05) {
                dbg_printf("identify PASS! pos:%d score:%d\n", pos, score);
                out_para[0] = *para; // 05
                out_para[1] = pkt->response; // 0
                break;
            }
        }
    }

    // fill out_para
    out_para[2] = pos;
    out_para[3] = score;

    as60x_ps_sleep();
    ccrReturn(AS60X_OPERATION_DEAL_PARTIAL);
    WAIT_FOR_RESPONSE(op->res_evt);
    if(op->res_evt->u.res_packet.response != RESPONSE_OK) {
        dbg_printf("identify: sleep failed!\n");
        ccrReturn(AS60X_OPERATION_FAILED);
    }
    as60x_cntx.state = AS60X_STATE_SENSOR_SLEEP;
    as60x_module_shutdown_for_sleep();

    ccrFinish(AS60X_OPERATION_COMPLETED);
}

static void as60x_register_finger_result_cb(as60x_operation_t *op, bool result) {
    if(result) {
        uint16_t *para = (uint16_t *)op->para.out_para;
        uint16_t stage = para[0];
        uint16_t code = para[1];
        if(stage == 5 && code == 0) {
            // todo
            dbg_printf("identify result OK!(pos:%d score:%d)\n", para[0], para[1]);
        } else {
            dbg_printf("identify result FAILED!(stage:%d code:%d)\n", stage, code);
        }
    } else {
        dbg_printf("identify operation failed!\n");
    }

}

static int32_t as60x_register_finger_coroutine(ccrContParam, as60x_operation_t *op) {
    ccrBeginContext;
    //int result;
    ccrEndContext(p);
    uint8_t *para;
    uint16_t pos, score;
    as60x_res_packet_t *pkt;
    uint16_t *out_para = (uint16_t *)op->para.out_para;
    register_finger_config_t *in_para = (register_finger_config_t *)op->para.in_para;

    ccrBegin(p);
    memset(out_para, 0, 8);

    as60x_module_bootup_resume();
    ccrReturn(AS60X_OPERATION_DEAL_PARTIAL);
    WAIT_FOR_BOOTFLAG(op->boot_flag);
    if(op->boot_flag != AS60X_BOOTUP_FLAG) {
        dbg_printf("bootup flag mismatch!(%x)\n", op->boot_flag);
        fp_module_reset();
        ccrReturn(AS60X_OPERATION_FAILED);
    }

    as60x_ps_auto_enroll(in_para->store_pos,
                        in_para->repeat_num,
                        in_para->led_mode,
                        in_para->pre_proc,
                        in_para->allow_override,
                        in_para->disallow_dup_finger,
                        in_para->continuous_capture);
    ccrReturn(AS60X_OPERATION_DEAL_PARTIAL);
    while(1) {
        WAIT_FOR_RESPONSE(op->res_evt);

        if(op->res_evt->u.res_packet.res_para.len != 5) {
            dbg_printf("identify: recv packet para lens mismatch failed!(%d)\n",
                        op->res_evt->u.res_packet.res_para.len);
            ccrReturn(AS60X_OPERATION_FAILED);
        }

        pkt = &(op->res_evt->u.res_packet);
        para = pkt->res_para.data;
        pos = (*(para+1))<<8 | (*(para+2));
        score = (*(para+3))<<8 | (*(para+4));
        if(pkt->response != RESPONSE_OK) {
            dbg_printf("identify: response failed!(%d, %d)\n", pkt->response, *para);
            out_para[0] = *para; // fail stage
            out_para[1] = pkt->response; // err code
            //ccrReturn(AS60X_OPERATION_FAILED);
            break;
        } else {
            if(*para == 0x00) {
                dbg_printf("identify: command validation check PASS\n");
                ccrReturn(AS60X_OPERATION_DEAL_PARTIAL);
            } else if(*para == 0x01) {
                dbg_printf("identify: geting image PASS\n");
                ccrReturn(AS60X_OPERATION_DEAL_PARTIAL);
            } else if(*para == 0x05) {
                dbg_printf("identify PASS! pos:%d score:%d\n", pos, score);
                out_para[0] = *para; // 05
                out_para[1] = pkt->response; // 0
                break;
            }
        }
    }

    // fill out_para
    out_para[2] = pos;
    out_para[3] = score;

    as60x_ps_sleep();
    ccrReturn(AS60X_OPERATION_DEAL_PARTIAL);
    WAIT_FOR_RESPONSE(op->res_evt);
    if(op->res_evt->u.res_packet.response != RESPONSE_OK) {
        dbg_printf("identify: sleep failed!\n");
        ccrReturn(AS60X_OPERATION_FAILED);
    }
    as60x_cntx.state = AS60X_STATE_SENSOR_SLEEP;
    as60x_module_shutdown_for_sleep();

    ccrFinish(AS60X_OPERATION_COMPLETED);
}

static void as60x_init_result_cb(as60x_operation_t *op, bool result) {
    dbg_printf("as60x_init_result_cb result:%d\n", result);
    if(result) {
    }

    fp_init_done_ind(result);
    //destroy_operation(op);
}

static int32_t as60x_init_coroutine(ccrContParam, as60x_operation_t *op) {
    struct as60x_config *cfg = (struct as60x_config *)(op->para.in_para);
    ccrBeginContext;
    //int i;
    ccrEndContext(p);
    ccrBegin(p);

    as60x_ps_handshake();
    ccrReturn(AS60X_OPERATION_DEAL_PARTIAL);
    WAIT_FOR_RESPONSE(op->res_evt);
    if(op->res_evt->u.res_packet.response != RESPONSE_OK) {
        dbg_printf("as60x_init_coroutine: handshake failed!\n");
        ccrReturn(AS60X_OPERATION_FAILED);
    }

    as60x_ps_sleep();
    ccrReturn(AS60X_OPERATION_DEAL_PARTIAL);
    WAIT_FOR_RESPONSE(op->res_evt);
    if(op->res_evt->u.res_packet.response != RESPONSE_OK) {
        dbg_printf("as60x_init_coroutine: handshake failed!\n");
        ccrReturn(AS60X_OPERATION_FAILED);
    }

    as60x_cntx.state = AS60X_STATE_SENSOR_SLEEP;
    as60x_module_shutdown_for_sleep();
    
    ccrFinish(AS60X_OPERATION_COMPLETED);
}

static int32_t as60x_check_handshake() {
    int32_t ret;
    as60x_operation_t *op;
    op_para_t para;
    
    op = build_operation(
            "handshake",
            as60x_handshake_coroutine,
            NULL,
            as60x_handshake_result_cb);
    if(op)
        add_single_operation(op);
    return 0;
}

int32_t as60x_delete_char(uint16_t id, op_result_cb_t result_cb) {
    //add_single_operation
    //run_operation(as60x_delete_char_coroutine, result_cb);
}

int32_t as60x_read_sys_info(uint8_t *value_list, op_result_cb_t result_cb) {
    //run_operation(as60x_handshake_coroutine, result_cb);
}

int32_t as60x_write_sys_reg(uint8_t regid, uint8_t regval, op_result_cb_t result_cb) {
    
    //run_operation(as60x_handshake_coroutine, result_cb);
}

static int32_t as60x_init_module() {
    int32_t ret;
    as60x_operation_t *op;
    op_para_t para;

    para.in_para = &as60x_init_config;
    para.out_para = NULL;
    op = build_operation(
            "module_init",
            as60x_init_coroutine,
            &para,
            as60x_init_result_cb);
    if(op)
        add_single_operation(op);
    return 0;
}

static void as60x_sensor_eint_hisr() {
    kal_uint8 temp;

    temp = GPIO_ReadIO(GPIO_FP_TOUCH_OUT_NUM);
    touch_out_state = !temp; // through a mosfet
    EINT_Set_Polarity(GPIO_FP_SENSOR_INT_NUM, touch_out_state);
    dbg_printf("as60x_sensor_eint_hisr: touch_out %d cntx_state %d\n", touch_out_state, as60x_cntx.state);

    if(touch_out_state == 1 && as60x_cntx.state == AS60X_STATE_SENSOR_SLEEP) {
        int32_t ret;
        as60x_operation_t *op;

        op = build_operation(
                "wake_idtfy",
                as60x_wakeup_identify_coroutine,
                &as60x_cntx.identify_op_para,
                as60x_wakeup_identify_result_cb);
        if(op)
            add_single_operation(op);

        fp_send_ilm_to_self();
    }
}

int32_t as60x_bootup_flag_recieved(uint8_t flag) {
    if(flag == AS60X_BOOTUP_FLAG) {
        dbg_printf("bootup flag revieved!\n");
        if(as60x_cntx.state == AS60X_STATE_POWER_ON) {
            as60x_cntx.state = AS60X_STATE_BOOTUP_COMPLETE;
            as60x_init_module();
        }
    } else {
        dbg_printf("bootup flag mismatch!(%x)\n", flag);
    }
    return 0;
}

int32_t fp_handle_transport_messages(ilm_struct *ilm) {
    if(as60x_cntx.state == AS60X_STATE_POWER_DOWN ||
            as60x_cntx.state == AS60X_STATE_SENSOR_SLEEP)
        return 0;

    switch(ilm->msg_id)
    {
        case MSG_ID_UART_READY_TO_READ_IND:
            as60x_transport_ready_to_read();
            break;
      
        case MSG_ID_UART_READY_TO_WRITE_IND:
            as60x_transport_ready_to_write();
            break;
    }

    return 0;
}


int32_t fp_handle_packet_event() {
    dbg_printf("fp_handle_packet_event state:%d\n", as60x_cntx.state);
    handle_packet_events();
}

int32_t fp_module_reset() {
    dbg_printf("fp_module_reset");
    //fp_module_exit();

    //as60x_cntx.dev_addr = AS60X_ADDR;
    as60x_cntx.state = AS60X_STATE_POWER_DOWN;
    as60x_cntx.current_op = NULL;
    clean_event_handler();
    clean_operations();

    EINT_Mask(GPIO_FP_SENSOR_INT_NUM);
    as60x_transport_stop();
    as60x_module_power_off();
    as60x_sensor_power_off();

    kal_sleep_task(KAL_TICKS_1_SEC);

    as60x_sensor_power_on();
    as60x_module_power_on();
    as60x_transport_wait_bootflag_on();

    init_event_handler();
    init_operations();

    as60x_transport_buff_init();
    as60x_transport_start(AS60X_UART_PORT_NUM, AS60X_UART_BAUDRATE);

    as60x_cntx.state = AS60X_STATE_POWER_ON;
}

int32_t fp_module_exit() {
    int32_t ret = 0;

    mem_free(as60x_cntx.identify_op_para.in_para);
    mem_free(as60x_cntx.identify_op_para.out_para);
    memset(&as60x_cntx, 0, sizeof(as60x_cntx));
    as60x_cntx.state = AS60X_STATE_POWER_DOWN;

    dbg_printf("fp_module_exit");
    EINT_Mask(GPIO_FP_SENSOR_INT_NUM);
    // todo: clean up allocated buffers
    //
    clean_event_handler();
    clean_operations();

    //as60x_transport_buff_init();
    as60x_transport_stop();
    as60x_module_power_off();
    as60x_sensor_power_off();

    return ret;
}

int32_t fp_module_init() {
    int32_t ret = 0;

    locker_init();

    memset(&as60x_cntx, 0, sizeof(as60x_cntx));
    as60x_cntx.state = AS60X_STATE_POWER_DOWN;
    //as60x_cntx.dev_addr = AS60X_ADDR;
    as60x_cntx.identify_op_para.in_para = mem_alloc(32);
    as60x_cntx.identify_op_para.out_para = mem_alloc(32);

    dbg_printf("fp_module_init");
    as60x_sensor_power_on();
    as60x_module_power_on();
    as60x_transport_wait_bootflag_on();
    as60x_register_touch_out_int();

    init_event_handler();
    init_operations();

    as60x_transport_buff_init();
    as60x_transport_start(AS60X_UART_PORT_NUM, AS60X_UART_BAUDRATE);

    as60x_cntx.state = AS60X_STATE_POWER_ON;

    return ret;
}

