#ifndef _TRANSPORT_H_
#define _TRANSPORT_H_

#include "ringbuffer.h"
#include "serial.h"


#pragma pack(push, 1)

typedef struct {
    uint8_t *data;
    uint8_t len;
} as60x_parameters;

typedef struct {
    //uint16_t tocken;
    uint32_t dev_addr;
    uint8_t type;
    uint16_t length;
} as60x_packet_head_t;

typedef struct {
    //uint16_t tocken;
    uint32_t dev_addr;
    uint8_t type;
    uint16_t length;
    uint8_t data[];
} as60x_recv_packet_common_t;

typedef struct {
    //uint16_t tocken;
    uint32_t dev_addr;
    uint8_t type;
    uint16_t length;
    uint8_t cmd;
    as60x_parameters cmd_para;
} as60x_cmd_packet_t;

typedef struct {
    //uint16_t tocken;
    uint32_t dev_addr;
    uint8_t type;
    uint16_t length;
    uint8_t response;
    as60x_parameters res_para;
} as60x_res_packet_t;

typedef struct {
    //uint16_t tocken;
    uint32_t dev_addr;
    uint8_t type;
    uint16_t length;
    uint8_t *data;
} as60x_data_packet_t;

#pragma pack(pop)


int32_t as60x_transport_buff_init();
int32_t as60x_transport_deinit();
int32_t as60x_transport_start(uint16_t port, uint32_t baudrate);
//int32_t as60x_transport_start_for_download(int16_t port, uint32_t baudrate);
int32_t as60x_transport_stop();
int32_t as60x_transport_trigger_write(void);
int32_t as60x_transport_enqueue_pkt(as60x_cmd_packet_t *msg);

// triggered from upper or lower layers
int32_t as60x_transport_ready_to_read();
int32_t as60x_transport_ready_to_write();

int32_t as60x_transport_write_poll(char *buff, int32_t len, int32_t timeout_ms);
int32_t as60x_transport_read_poll(char *buff, int32_t len, int32_t timeout_ms);
int32_t as60x_transport_write(char *buff, int32_t len, int32_t timeout_ms);
int32_t as60x_transport_read(char *buff, int32_t len, int32_t timeout_ms);
int32_t as60x_transport_purge_rx(uint16_t port);

void as60x_transport_wait_bootflag_on();

#endif

