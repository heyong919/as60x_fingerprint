#ifndef _HW_CONFIG_H_
#define _HW_CONFIG_H_

#include "dcl_uart.h"

// for fingerprint module
#define GPIO_FP_3V3_POWER_EN       (27)
#define GPIO_FP_SENSOR_POWER_EN    (32)
#define GPIO_FP_AS60X_POWER_EN     (33)
#define GPIO_FP_SENSOR_INT_NUM     (23)
#define GPIO_FP_TOUCH_OUT_NUM      (52)

#define GPIO_FP_UTXD_PIN           (11)
#define GPIO_FP_URXD_PIN           (10)
#define AS60X_UART_PORT_NUM        (uart_port1)
#define AS60X_UART_BAUDRATE        (57600)

#define AS60X_DEVICE_ADDR          (0xFFFFFFFF) // default 0xffffffff

#define AS60X_SECURITY_LEVEL       (3) // 1(lowest) - 5(highest)

#define DUMP_SERIAL_DATA           1


// for locker motor
#define MOTOR_DRV8837	// DRV8838

#define LOCKER_SENSE_LOCK_EINT_NO       (17) //gpio34 EINT17 MCDA1
#define LOCKER_SENSE_UNLOCK_EINT_NO     (18) //gpio35 EINT18 MCDA2
#define GPIO_LOCKER_SENSE_LOCK_PIN      (34)
#define GPIO_LOCKER_SENSE_UNLOCK_PIN    (35)
#define GPIO_MOTO_POWER_EN              (31)
#define GPIO_MOTORDRV_NSLEEP            (21)
#define GPIO_MOTORDRV_IN1               (40)
#define GPIO_MOTORDRV_IN2               (42)


#ifdef MOTOR_2

#define GPIO_MOTORDRV2_NSLEEP            (16)
#define GPIO_MOTORDRV2_IN1               (55)
#define GPIO_MOTORDRV2_IN2               (54)

#endif

#endif

