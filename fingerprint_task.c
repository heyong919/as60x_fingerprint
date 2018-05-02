/*
 ============================================================================
 Name        : fp_host_app.c
 Author      : hey
 Version     :
 Copyright   : Your copyright notice
 ============================================================================
 */
#ifdef __FINGERPRINT_TASK_SUPPORT__

#include "task_config.h"
#include "syscomp_config.h"
#include "drv_comm.h"
#include "dcl_uart.h"
#include "transport.h"

extern int32_t fp_handle_transport_messages(ilm_struct *ilm);
extern int32_t fp_module_init();
extern int32_t fp_module_reset();
extern int32_t fp_handle_packet_event();
extern int32_t fp_check_operations();
extern bool pending_events();
extern bool pending_operations();


static bool fp_init_done = false;

void fp_send_ilm_to_self() {  
    ilm_struct *ilm_ptr;
    
    DRV_BuildPrimitive(ilm_ptr,
     MOD_DRV_HISR,
     MOD_FP,
     MSG_ID_FINGERPRINT_TRIGGER,
     NULL);
    msg_send_ext_queue(ilm_ptr);
}

static void fp_send_ilm(module_type dst, msg_type msgid, local_para_struct *local_para_ptr) {
    ilm_struct *ilm_ptr;
    
    DRV_BuildPrimitive(ilm_ptr,
     MOD_FP,
     dst,
     msgid,
     local_para_ptr);
    msg_send_ext_queue(ilm_ptr);
}

static void fp_handle_int_event(ilm_struct *ilm) {
}

void fp_init_done_ind(bool res) {
    fp_init_done = res;
    fp_send_ilm(MOD_JOMA, MSG_ID_FINGERPRINT_INIT_RSP, NULL);
}



#if defined(__MTK_TARGET__) && defined(__DCM_WITH_COMPRESSION_MAUI_INIT__)
#pragma push
#pragma arm section code="DYNAMIC_COMP_MAUIINIT_SECTION"
#endif

kal_bool fp_task_init(task_indx_type task_indx)
{
  /* Do task's initialization here.
   * Notice that: shouldn't execute modules reset handler since 
   * stack_task_reset() will do. */
  return KAL_TRUE;
}

#if defined(__MTK_TARGET__) && defined(__DCM_WITH_COMPRESSION_MAUI_INIT__)
#pragma arm section code
#pragma pop
#endif

kal_bool fp_task_reset(task_indx_type task_indx)
{
  /* Do task's reset here.
   * Notice that: shouldn't execute modules reset handler since 
   * stack_task_reset() will do. */
  return KAL_TRUE;
}

kal_bool fp_task_end(task_indx_type task_indx)
{
  /* Do task's termination here.
   * Notice that: shouldn't execute modules reset handler since 
   * stack_task_end() will do. */
  return KAL_TRUE;
}

void fp_task_main(task_entry_struct *task_entry_ptr)
{
    ilm_struct current_ilm;
    kal_uint32 my_index;     

    kal_get_my_task_index(&my_index);     
    stack_set_active_module_id(my_index, MOD_FP);

    //fp_module_init();

    while(1)
    {
        receive_msg_ext_q_for_stack(task_info_g[task_entry_ptr->task_indx].task_ext_qid, &current_ilm);

        fp_handle_transport_messages(&current_ilm);

        switch(current_ilm.msg_id)
        {
            case MSG_ID_FINGERPRINT_INIT_REQ:
            {
                if(fp_init_done) {
                    fp_send_ilm(current_ilm.src_mod_id, MSG_ID_FINGERPRINT_INIT_RSP, NULL);
                } else {
                    fp_module_init();
                }
            }
            break;

            case MSG_ID_FINGERPRINT_RESET_REQ:
            {
                fp_module_reset();
            }
            break;

            case MSG_ID_FINGERPRINT_REG_FINGER_REQ:
            {
            }
            break;

            /*
            case MSG_ID_BLE_GAP_SET_ROLE:
            {
            user_operation_t set_role_ops;
            fp_set_role_t *set_role = (fp_set_role_t *)(current_ilm.local_para_ptr);

            set_role_ops.event_id = GAPM_CMP_EVT;
            set_role_ops.handler = NULL;
            if(set_role->role == GAP_ROLE_PERIPHERAL) {
            set_role_ops.command = user_set_dev_config_perpheral;
            } else {
            set_role_ops.command = user_set_dev_config_central;
            }
            app_add_single_operation(&set_role_ops);
            }
            break;

            MSG_ID_FINGERPRINT_REG_FINGER_REQ,
            MSG_ID_FINGERPRINT_REG_FINGER_IND,
            MSG_ID_FINGERPRINT_REG_FINGER_RSP,

            MSG_ID_FINGERPRINT_VERIFY_FINGER_REQ,
            MSG_ID_FINGERPRINT_VERIFY_FINGER_RSP,

            MSG_ID_FINGERPRINT_CANCEL_OPERATION_REQ,
            MSG_ID_FINGERPRINT_CANCEL_OPERATION_RSP,

            MSG_ID_FINGERPRINT_DELETE_CHAR_REQ,
            MSG_ID_FINGERPRINT_DELETE_CHAR_RSP,

            */
            
            case MSG_ID_FINGERPRINT_TRIGGER:
            {
            }
            break;

            default:
            break;
        }
        free_ilm(&current_ilm);

        
        while(pending_events() || pending_operations()) {

            fp_handle_packet_event();

            fp_check_operations();

            //fp_handle_int_event(&current_ilm);
        }

    }
}

kal_bool fp_create(comptask_handler_struct **handle)
{
  static const comptask_handler_struct fp_handler_info = 
  {
    fp_task_main,      /* task entry function */
    fp_task_init,      /* task initialization function */
    NULL,   /* task configuration function */
    fp_task_reset,   /* task reset handler */
    fp_task_end      /* task termination handler */
  };
  
  *handle = (comptask_handler_struct *)&fp_handler_info;
  return KAL_TRUE;
}

#endif

