#include <stdio.h>
#include <stdlib.h>
#include "as60x.h"

#undef QUEUE_TYPE
#define QUEUE_TYPE  as60x_event_t*
#include "queue.h"

#define MAX_EVENT_SIZE (8)
static as60x_event_t* event_queue_buffer[MAX_EVENT_SIZE];
static queue_t event_queue;


int32_t set_data_packet_buffer() {
}


int32_t submit_packet_event(as60x_event_t *pkt_evt) {
    int32_t ret = enqueue_tail(&event_queue, &pkt_evt);
    return ret;
}

int32_t pkt_recv_bootup_flag(uint8_t flag) {
#if 0
    as60x_event_t *bootup_evt = (as60x_event_t *)mem_alloc(sizeof(as60x_event_t));
    
    if(bootup_evt != NULL) {
        bootup_evt->boot_flag = flag;    
        submit_packet_event(bootup_evt);
    } else {
        dbg_printf("bootup_evt mem_alloc FAILED!\n");
    }
    handle_packet_events();
#else
    as60x_bootup_flag_recieved(flag);
    operation_handle_bootup_flag(flag);
#endif
}

int32_t pkt_recv_handler(as60x_recv_packet_common_t *pkt) {

    dbg_printf("received pkt tppe(%x)(%d)(%d)\n", pkt->dev_addr, pkt->type, pkt->length);

    switch(pkt->type) {
        case PACKET_TYPE_RESPONSE:
        {
            as60x_res_packet_t *res_pkt = (as60x_res_packet_t *)pkt;
            uint8_t para_len = res_pkt->length - 1 - PKT_CHKSUM_SIZE; // 1B confirm code
            as60x_event_t *res_evt = (as60x_event_t *)mem_alloc(sizeof(as60x_event_t) + para_len);
            uint8_t *para = (uint8_t *)res_evt + sizeof(as60x_event_t);

            if(res_evt != NULL) {
                res_evt->boot_flag = 0;
                memcpy(&res_evt->u.res_packet, res_pkt, sizeof(as60x_res_packet_t));
                memcpy(para, pkt->data+1, para_len);
                res_evt->u.res_packet.response = pkt->data[0];
                res_evt->u.res_packet.res_para.len = para_len;
                res_evt->u.res_packet.res_para.data = para;

                submit_packet_event(res_evt);
            } else {
                dbg_printf("res_evt mem_alloc FAILED!\n");
            }
            break;
        }
        case PACKET_TYPE_DATA:
        {
            // copy data to buffer given by set_data_packet_buffer
            break;
        }
        case PACKET_TYPE_DATA_END:
        {
            // copy data to buffer given by set_data_packet_buffer
            // submit data packet event
            break;
        }

        default:
            dbg_printf("unknown packet type recved!\n");
    }

    // handle packet event if any
    //handle_packet_events();
    return 0;
}

int32_t handle_packet_events() {
    as60x_event_t *evt;
    while((evt=dequeue(&event_queue)) != NULL) {
        operation_handle_event(evt);
    }
}

bool pending_events() {
    return !queue_empty(&event_queue);
}

int32_t clean_event_handler() {
    queue_init(&event_queue, event_queue_buffer, MAX_EVENT_SIZE);
    return 0;
}


int32_t init_event_handler() {
    queue_init(&event_queue, event_queue_buffer, MAX_EVENT_SIZE);
    return 0;
}

