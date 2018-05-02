#ifndef _OPERATION_H_
#define _OPERATION_H_

#include "coroutine.h"
#include "stdtypes.h"
#include "transport.h"

#define OP_NAME_SIZE  16

struct _as60x_operation_t;
typedef void (*op_result_cb_t)(struct _as60x_operation_t *op, bool result);

typedef int32_t (*op_coroutine_t)(ccrContext c, struct _as60x_operation_t *op);

typedef struct {
    uint8_t boot_flag;
    union {
        as60x_packet_head_t packet_head;
        as60x_res_packet_t res_packet;
        as60x_data_packet_t data_packet;
    } u;
} as60x_event_t;

typedef struct {
    void *in_para;
    void *out_para;
} op_para_t;

typedef struct _as60x_operation_t{
    ccrContext ctx;
    char_t name[OP_NAME_SIZE];
    op_coroutine_t process;
    op_para_t para;
    op_result_cb_t res_cb;
    as60x_event_t *res_evt;
    uint8_t boot_flag;
} as60x_operation_t;

typedef enum {
    AS60X_OPERATION_COMPLETED = 0,
    AS60X_OPERATION_DEAL_PARTIAL = 1,
    AS60X_OPERATION_FAILED = -1,
};

#define WAIT_FOR_RESPONSE(res_evt) \
    do { \
        while(res_evt == NULL) \
            ccrReturn(AS60X_OPERATION_DEAL_PARTIAL); \
        dbg_printf("resume coroutine for response...\n"); \
    } while(0)

#define WAIT_FOR_BOOTFLAG(flag) \
    do { \
        while(flag == NULL) \
            ccrReturn(AS60X_OPERATION_DEAL_PARTIAL); \
        dbg_printf("resume coroutine for bootflag...\n"); \
    } while(0)

int32_t add_single_operation(as60x_operation_t *op);
as60x_operation_t* build_operation(char_t *name, 
                            op_coroutine_t process, 
                            op_para_t *para,
                            op_result_cb_t res_cb);
void destroy_operation(as60x_operation_t *op);
int32_t fp_check_operations();
int32_t operation_handle_bootup_flag(uint8_t flag);
int32_t operation_handle_event(as60x_event_t *evt);
bool pending_operations();
int32_t clean_operations();
int32_t init_operations();

#endif

