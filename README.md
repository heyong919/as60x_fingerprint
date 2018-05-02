# as60x_fingerprint
as60x fingerprint driver on Mediatek MT2503 platform

## HW
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; UART,GPIO &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; SPI,GPIO

MCU <-----------------> AS60x <---------------> FPC1021

&nbsp;&nbsp;&nbsp; |&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; |

&nbsp;&nbsp;&nbsp; <----------------------------------------------------------

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; IRQ

## SW
platform: MTK MAUI

## How to build
1. make\fingerprint\fingerprint.mak
```
INC_DIR += ...
SRC_LIST += xxx.c ...
COMP_DEFS += ...
```
2. interface\fingerprint\fingerprint_sap.h
```
MSG_ID_FINGERPRINT_RESET_REQ = MSG_ID_FINGERPRINT_CODE_BEGIN,
MSG_ID_FINGERPRINT_RESET_RSP,
MSG_ID_FINGERPRINT_INIT_REQ,
MSG_ID_FINGERPRINT_INIT_RSP,
MSG_ID_FINGERPRINT_REG_FINGER_REQ,
MSG_ID_FINGERPRINT_REG_FINGER_IND,
MSG_ID_FINGERPRINT_REG_FINGER_RSP,
MSG_ID_FINGERPRINT_VERIFY_FINGER_REQ,
MSG_ID_FINGERPRINT_VERIFY_FINGER_RSP,
......
```
3. goto top of MAUI package
```
make r fingerprint
```
or 
```
make new
```
