#include <stdio.h>
#include <stdlib.h>
#include "as60x.h"
#include "coroutine.h"


#undef QUEUE_TYPE
#define QUEUE_TYPE  as60x_operation_t*
#include "queue.h"

#define OPS_QUEUE_SIZE (8)
static as60x_operation_t *ops_queue_buffer[OPS_QUEUE_SIZE];
static queue_t ops_queue;
static as60x_operation_t *current_op;
static uint8_t op_state;

extern as60x_context_t as60x_cntx;

enum {
    FLAG_NEXT_IN_LIST,
    FLAG_RETRY_LIST,
    FLAG_NEXT_IN_QUEUE
};

enum {
    OP_STATE_IDLE,
    OP_STATE_RUNING,
    OP_STATE_WAITING
};







static int32_t execute_operation(as60x_operation_t *op) {
    int32_t ret;

    dbg_printf("execute_operation %s\n", op->name);
    ret = op->process(&op->ctx, op);

    if(op->res_evt) {
        mem_free(op->res_evt);
        op->res_evt = NULL;
    }

    if(ret == AS60X_OPERATION_DEAL_PARTIAL) {
        // still have further works to do
        // so wait for event to continue processing

    } else if(ret == AS60X_OPERATION_COMPLETED) {
        // all works have been done
        // issue callback
        op->res_cb(op, true);
        destroy_operation(op);
        //next_operation();
    } else if(ret == AS60X_OPERATION_FAILED){
        // failed to process
        ccrAbort(current_op->ctx);
        op->res_cb(op, false);
        destroy_operation(op);
        dbg_printf("op->process %s failed!\n", op->name);
    }
    return ret;
}

static int32_t next_operation() {
    int32_t ret;

    current_op = dequeue(&ops_queue);
    while(current_op) {
        op_state = OP_STATE_RUNING;
        ret = execute_operation(current_op);
        if(ret == AS60X_OPERATION_COMPLETED || ret == AS60X_OPERATION_FAILED) {
            // operation done
            current_op = dequeue(&ops_queue);
            if(current_op == NULL) {
                dbg_printf("ops_queue empty");
                op_state = OP_STATE_IDLE;
                break;
            }
        } else if(ret == AS60X_OPERATION_DEAL_PARTIAL) {
            // wait event for further work
            op_state = OP_STATE_WAITING;
            break;
        }
    }

    if(current_op == NULL) {
        dbg_printf("ops_queue empty");
        op_state = OP_STATE_IDLE;
    }
    return ret;
}

static int32_t start_operations() {
    int32_t ret;
    if(current_op == NULL && op_state == OP_STATE_IDLE) {
        next_operation();
    }
    return 0;
}

int32_t add_single_operation(as60x_operation_t *op) {
    if(op) {
        if(enqueue_tail(&ops_queue, &op) == 0) {
            if(!current_op) {
                start_operations();
            }
            return 0;
        }
        else {
          dbg_printf("enqueue failed!\n");
        }
    }
}

as60x_operation_t* build_operation(char_t *name, 
                            op_coroutine_t process, 
                            op_para_t *para,
                            op_result_cb_t res_cb) {
    as60x_operation_t *op;
    op = mem_alloc(sizeof(as60x_operation_t));
    if(op) {
        //op->ctx = 0;
        memset(op, 0, sizeof(as60x_operation_t));
        memcpy(op->name, name, strlen(name));
        op->name[strlen(name)] = '\0';
        op->process = process;
        if(para) {
            op->para.in_para = para->in_para;
            op->para.out_para = para->out_para;
        }
        op->res_cb = res_cb;
    } else {
        dbg_printf("build_operation mem_alloc failed!\n");
    }
    return op;
}

void destroy_operation(as60x_operation_t *op) {
    if(op) {
        if(op->res_evt) {
            mem_free(op->res_evt);
            op->res_evt = NULL;
        }
        mem_free(op);
    }
}

int32_t fp_check_operations() {
    dbg_printf("fp_check_operations state:%d\n", as60x_cntx.state);
    start_operations();
}

int32_t operation_handle_bootup_flag(uint8_t flag) {
    int32_t ret;
    if(!current_op)
        return 0;

    current_op->boot_flag = flag;
    ret = execute_operation(current_op);
    if(ret != AS60X_OPERATION_DEAL_PARTIAL) {
        next_operation();
    }

    return 0;
}

int32_t operation_handle_event(as60x_event_t *evt) {
    int32_t ret;
    if(!current_op)
        return 0;

    current_op->res_evt = evt;
    ret = execute_operation(current_op);
    if(ret != AS60X_OPERATION_DEAL_PARTIAL) {
        next_operation();
    }

    return 0;
}

bool pending_operations() {
    return (!queue_empty(&ops_queue)) && (op_state == OP_STATE_IDLE);
}

int32_t clean_operations() {    
    queue_init(&ops_queue, ops_queue_buffer, OPS_QUEUE_SIZE);
    if(current_op) {
        ccrAbort(current_op->ctx);
        current_op = NULL;
    }
    op_state = OP_STATE_IDLE;
    return 0;
}

int32_t init_operations() {    
    queue_init(&ops_queue, ops_queue_buffer, OPS_QUEUE_SIZE);
    current_op = NULL;
    op_state = OP_STATE_IDLE;
    return 0;
}

