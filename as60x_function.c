
#include "stdtypes.h"
#include "as60x.h"
#include "transport.h"

/// 1) PS_GetImage
#define PS_GetImage (0x01)
/// ? 指令代码：01H
/// ? 功能：验证用获取图像
/// 2) PS_GetEnrollImage
#define PS_GetEnrollImage (0x29)
/// ? 指令代码：29H
/// ? 功能：注册用获取图像
/// 3) PS_GenChar
#define PS_GenChar (0x02)
/// ? 指令代码：02H
/// ? 功能：根据原始图像生成指纹特征存于特征文件缓冲区
/// 4) PS_Match
#define PS_Match (0x03)
/// ? 指令代码：03H
/// ? 功能：精确比对特征文件缓冲区中的特征文件
/// 5) PS_Search
#define PS_Search (0x04)
/// ? 指令代码：04H
/// ? 功能：以特征文件缓冲区中的特征文件搜索整个或部分指纹库
/// 6) PS_RegModel
#define PS_RegModel (0x05)
/// ? 指令代码：05H
/// ? 功能：将特征文件合并生成模板存于特征文件缓冲区
/// 7) PS_StoreChar
#define PS_StoreChar (0x06)
/// ? 指令代码：06H
/// ? 功能：将特征缓冲区中的文件储存到 flash 指纹库中
/// 8) PS_LoadChar
#define PS_LoadChar (0x07)
/// ? 指令代码：07H
/// ? 功能：从 flash 指纹库中读取一个模板到特征缓冲区
/// 9) PS_UpChar
#define PS_UpChar (0x08)
/// ? 指令代码：08H
/// ? 功能：将特征缓冲区中的文件上传给上位机
/// 10) PS_DownChar
#define PS_DownChar (0x09)
/// ? 指令代码：09H
/// ? 功能：从上位机下载一个特征文件到特征缓冲区
/// 11) PS_UpImage
#define PS_UpImage (0x0a)
/// ? 指令代码：0aH
/// ? 功能：上传原始图像
/// 12) PS_DownImage
#define PS_DownImage (0x0b)
/// ? 指令代码：0bH
/// ? 功能：下载原始图像
/// 13) PS_DeletChar
#define PS_DeletChar (0x0c)
/// ? 指令代码：0cH
/// ? 功能：删除 flash 指纹库中的一个特征文件
/// 14) PS_Empty
#define PS_Empty (0x0d)
/// ? 指令代码：0dH
/// ? 功能：清空 flash 指纹库
/// 15) PS_WriteReg
#define PS_WriteReg (0x0e)
/// ? 指令代码：0eH
/// ? 功能：写 SOC 系统寄存器
/// 16) PS_ReadSysPara
#define PS_ReadSysPara (0x0F)
/// ? 指令代码：0FH
/// ? 功能：读系统基本参数
/// 17) PS_AutoEnroll
#define PS_AutoEnroll (0x31)
/// ? 指令代码：31H
/// ? 功能：自动注册模板
/// 18) PS_AutoIdentify
#define PS_AutoIdentify (0x32)
/// ? 指令代码：32H
/// ? 功能：自动验证指纹
/// 19) PS_SetPwd
#define PS_SetPwd (0x12)
/// ? 指令代码：12H
/// ? 功能：设置设备握手口令
/// 20) PS_VfyPwd
#define PS_VfyPwd (0x13)
/// ? 指令代码：13H
/// ? 功能：验证设备握手口令
/// 21) PS_GetRandomCode
#define PS_GetRandomCode (0x14)
/// ? 指令代码：14H
/// ? 功能：采样随机数
/// 22) PS_SetChipAddr
#define PS_SetChipAddr (0x15)
/// ? 指令代码：15H
/// ? 功能：设置芯片地址
/// 23) PS_ReadINFpage
#define PS_ReadINFpage (0x16)
/// ? 指令代码：16H
/// ? 功能：读取 FLASH Information Page 内容
/// 24) PS_Port_Control
#define PS_Port_Control (0x17)
/// ? 指令代码：17H
/// ? 功能：通讯端口（UART/USB）开关控制
/// 25) PS_WriteNotepad
#define PS_WriteNotepad (0x18)
/// ? 指令代码：18H
/// ? 功能：写记事本
/// 26) PS_ReadNotepad
#define PS_ReadNotepad (0x19)
/// ? 指令代码：19H
/// ? 功能：读记事本
/// 27) PS_BurnCode （AS60x SOC 该指令为烧写片外 FLASH 代码）
#define PS_BurnCode (0x1a)
/// ? 指令代码：1aH
/// ? 功能：烧写片内 FLASH
/// 28) PS_HighSpeedSearch
#define PS_HighSpeedSearch (0x1b)
/// ? 指令代码：1bH
/// ? 功能：高速搜索 FLASH
/// 29) PS_GenBinImage
#define PS_GenBinImage (0x1c)
/// ? 指令代码：1cH
/// ? 功能：生成二值化指纹图像
/// 30) PS_ValidTempleteNum
#define PS_ValidTempleteNum (0x1d)
/// ? 指令代码：1dH
/// ? 功能：读有效模板个数
/// 31) PS_UserGPIOCommand
#define PS_UserGPIOCommand (0x1e)
/// ? 指令代码：1eH
/// ? 功能：用户 GPIO 控制命令
/// 32) PS_ReadIndexTable
#define PS_ReadIndexTable (0x1f)
/// ? 指令代码：1fH
/// ? 功能：读索引表
/// 33) PS_Cancle
#define PS_Cancle (0x30)
/// ? 指令代码：30H
/// ? 功能：取消指令
/// 34) PS_AutoEnroll
#define PS_AutoEnroll (0x31)
/// ? 指令代码：31H
/// ? 功能：自动注册模块指令
/// 35) PS_AutoIdentify
#define PS_AutoIdentify (0x32)
/// ? 指令代码：32H
/// ? 功能：自动验证指纹指令
/// 36) PS_Sleep
#define PS_Sleep (0x33)
/// ? 指令代码：33H
/// ? 功能：休眠指令
/// 37) PS_GetChipSN
#define PS_GetChipSN (0x34)
/// ? 指令代码：34H
/// ? 功能：获取芯片唯一序列号
/// 38) PS_HandShake
#define PS_HandShake (0x35)
/// ? 指令代码：35H
/// ? 功能：握手指令
/// 39) PS_CheckSensor
#define PS_CheckSensor (0x36)
/// ? 指令代码：36H
/// ? 功能：校验传感器

int32_t build_cmd_packet(
            as60x_cmd_packet_t *pkt,
            uint32_t dev_addr, uint8_t type, 
            uint8_t cmd, uint8_t *para, uint8_t para_len) {
    memset(pkt, 0, sizeof(as60x_cmd_packet_t));
    pkt->dev_addr = dev_addr;
    pkt->type = type;
    pkt->cmd = cmd;
    if(para && para_len>0) {
        pkt->cmd_para.data = para;
        pkt->cmd_para.len = para_len;
    }
    pkt->length = PKT_COMMAND_SIZE + para_len + PKT_CHKSUM_SIZE;
}

int32_t free_cmd_packet(as60x_cmd_packet_t *pkt) {
    if(pkt->cmd_para.data && pkt->cmd_para.len) {
        mem_free(pkt->cmd_para.data);
    }
    mem_free(pkt);
    return 0;
}

int32_t as60x_ps_handshake() {
    as60x_cmd_packet_t *pkt;

    dbg_printf("as60x_ps_handshake\n");
    pkt = mem_alloc(sizeof(as60x_cmd_packet_t));
    if(pkt) {
        build_cmd_packet(pkt, 
                AS60X_DEVICE_ADDR, 
                PACKET_TYPE_COMMAND, 
                PS_HandShake,
                NULL,
                0);
        as60x_transport_enqueue_pkt(pkt);
    }
}

int32_t as60x_ps_sleep() {
    as60x_cmd_packet_t *pkt;
    
    dbg_printf("as60x_ps_sleep\n");
    pkt = mem_alloc(sizeof(as60x_cmd_packet_t));
    if(pkt) {
        build_cmd_packet(pkt, 
                AS60X_DEVICE_ADDR, 
                PACKET_TYPE_COMMAND, 
                PS_Sleep,
                NULL,
                0);
        as60x_transport_enqueue_pkt(pkt);
    }
}

int32_t as60x_ps_set_addr(uint32_t addr) {
}

int32_t as60x_ps_auto_enroll(
            uint16_t pos,
            uint8_t num,
            uint8_t led,
            uint8_t prep,
            uint8_t allow_override,
            uint8_t disallow_dup,
            uint8_t non_leave
            ) {
    uint16_t para16;
    uint8_t *pkt_para;
    as60x_cmd_packet_t *pkt;

    dbg_printf("as60x_ps_auto_enroll %d %d %d %d %d %d %d\n",
        pos, num, led, prep, allow_override, allow_dup, leave);
    pkt = mem_alloc(sizeof(as60x_cmd_packet_t));
    if(pkt) {
        pkt_para = mem_alloc(5);
        if(!pkt_para) {
            dbg_printf("as60x_ps_auto_enroll alloc para failed!\n");
            return -2;
        }
        pkt_para[0] = pos>>8;
        pkt_para[1] = pos&0xFF;
        pkt_para[2] = num;
        para16 = (led&0x1) | (prep&0x1)<<1 | (0<<2) | (allow_override&0x1)<<3 |
                 (disallow_dup&0x1)<<4 | (non_leave&0x1)<<5;
        pkt_para[3] = para16>>8;
        pkt_para[4] = para16&0xFF;

        build_cmd_packet(pkt, 
                AS60X_DEVICE_ADDR, 
                PACKET_TYPE_COMMAND, 
                PS_AutoEnroll,
                pkt_para,
                5);
        as60x_transport_enqueue_pkt(pkt);
    } else {
        dbg_printf("as60x_ps_auto_enroll alloc packet failed!\n");
        return -1;
    }
}

int32_t as60x_ps_auto_identify(
            uint8_t sec_level,
            uint16_t pos,
            uint8_t led,
            uint8_t prep
            ) {
    uint16_t para16;
    uint8_t *pkt_para;
    as60x_cmd_packet_t *pkt;

    dbg_printf("as60x_ps_auto_identify %d %d %d %d\n", sec_level, pos, led, prep);
    pkt = mem_alloc(sizeof(as60x_cmd_packet_t));
    if(pkt) {
        pkt_para = mem_alloc(5);
        if(!pkt_para) {
            dbg_printf("as60x_ps_auto_identify alloc para failed!\n");
            return -2;
        }
        pkt_para[0] = sec_level;
        pkt_para[1] = pos>>8;
        pkt_para[2] = pos&0xFF;
        para16 = (led&0x1) | (prep&0x1)<<1 | (0<<2);
        pkt_para[3] = para16>>8;
        pkt_para[4] = para16&0xFF;

        build_cmd_packet(pkt, 
                AS60X_DEVICE_ADDR, 
                PACKET_TYPE_COMMAND, 
                PS_AutoIdentify,
                pkt_para,
                5);
        as60x_transport_enqueue_pkt(pkt);
    } else {
        dbg_printf("as60x_ps_auto_identify alloc packet failed!\n");
        return -1;
    }
}

