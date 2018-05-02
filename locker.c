
#include "kal_general_types.h"
#include "kal_public_api.h"
#include "stdtypes.h"
#include "locker.h"
#include "hw_config.h"

#define EINT_FLAG_OPEN_SW        1
#define EINT_FLAG_CLOSE_SW       2

#define LOCKER_FAILED_TH           10

#define get_lock_failed_count(count)    (count&0xFFF)
#define get_unlock_failed_count(count)  (count>>16)

static kal_timerid motor_timer;
static kal_timerid motor_sleep_timer;
static kal_uint32 motor_protect_flag=0xf0f0;
static kal_uint16 locker_hisr_state=0;
static kal_uint16 motor_runing_state=0;  //1:unlock 2:lock
static kal_uint16 motor_position;

static uint32_t lock_internal_state;
static uint32_t locker_initialized = false;

kal_uint32 t1;
kal_uint32 locker_failed_count=0;

static void delayms(kal_uint16 data)
{
    kal_uint32 time1;
    
    time1 = drv_get_current_time();
    while (drv_get_duration_tick(time1, drv_get_current_time()) <= (data*32768/1000));
}

static kal_bool get_lock_sense_switch_state()
{
    return (kal_bool)!GPIO_ReadIO(GPIO_LOCKER_SENSE_LOCK_PIN);
}

static kal_bool get_unlock_sense_switch_state()
{
    return (kal_bool)!GPIO_ReadIO(GPIO_LOCKER_SENSE_UNLOCK_PIN);
}

static uint32_t get_lock_int_state() {
    uint8_t open_st = get_unlock_sense_switch_state();
    uint8_t close_st = get_lock_sense_switch_state();
    uint32_t st = open_st+close_st;
    if(st > 1) {
        lock_internal_state = LOCK_INT_STATE_CONFLICT;
    } else if(st==0) {
        lock_internal_state = LOCK_INT_STATE_INTERMEDIATE;
    } else {
        if(open_st)
            lock_internal_state = LOCK_INT_STATE_UNLOCKED;
        else
            lock_internal_state = LOCK_INT_STATE_LOCKED;
    }
    dbg_printf("get_lock_int_state: %d (%d %d)", lock_internal_state, open_st, close_st);
    return lock_internal_state;
}

static void check_motor_position() {
	if(lock_internal_state == LOCK_INT_STATE_LOCKED)
		motor_position = LOCK_POS;
	else if(lock_internal_state == LOCK_INT_STATE_UNLOCKED)
		motor_position = UNLOCK_POS;
    else if(lock_internal_state == LOCK_INT_STATE_INTERMEDIATE)
        motor_position = INTERMEDIATE_POS;
    else
        motor_position = UNKNOWN_POS;
}

static void motor_start(kal_uint8 dir)
{
    //DCL_HANDLE gpioh_power_en, gpioh_moto_en, gpioh_moto_ph, gpioh_moto_in1, gpioh_moto_in2, gpioh_moto_sleep;

	if(dir!=DIR_LOCK && dir!=DIR_UNLOCK)
		return;

    //get_lock_int_state();
    check_motor_position();

	dbg_printf("motor_start(%d) cur pos(%d)", dir, motor_position);
	kal_cancel_timer(motor_sleep_timer);
	if(dir == DIR_LOCK)
	{
		motor_runing_state = MOTOR_ACTION_LOCK;
		if(motor_position == LOCK_POS)
			dbg_printf("warning!!! already in locked position!");
	}
	else
	{
		motor_runing_state = MOTOR_ACTION_UNLOCK;
		if(motor_position == UNLOCK_POS)
			dbg_printf("warning!!! already in unlocked position!");
	}
#ifdef MOTOR_DRV8837
    // turn on motor power
    GPIO_WriteIO(1, GPIO_MOTO_POWER_EN);
    delayms(10);

	// sleep first
	//DclGPIO_Control(gpioh_moto_sleep, GPIO_CMD_WRITE_LOW, NULL);
	GPIO_WriteIO(0, GPIO_MOTORDRV_NSLEEP);

    // brake motor
	GPIO_WriteIO(1, GPIO_MOTORDRV_IN1);
	GPIO_WriteIO(1, GPIO_MOTORDRV_IN2);
	GPIO_WriteIO(1, GPIO_MOTORDRV_NSLEEP);
	delayms(10); // >30us

	t1= drv_get_current_time();
	if(dir==DIR_LOCK)
	{
        //EINT_AckInt(LOCKER_SENSE_LOCK_EINT_NO);
        EINT_UnMask(LOCKER_SENSE_LOCK_EINT_NO);
		//DclGPIO_Control(gpioh_moto_in1, GPIO_CMD_WRITE_LOW, NULL);
		GPIO_WriteIO(0, GPIO_MOTORDRV_IN1);
	}
	else if(dir==DIR_UNLOCK)
	{
        //EINT_AckInt(LOCKER_SENSE_UNLOCK_EINT_NO);
        EINT_UnMask(LOCKER_SENSE_UNLOCK_EINT_NO);
		//DclGPIO_Control(gpioh_moto_in2, GPIO_CMD_WRITE_LOW, NULL);
		GPIO_WriteIO(0, GPIO_MOTORDRV_IN2);
	}

#endif

}

static void motor_driver_sleep(void *para)
{
	kal_uint16 retry=0;
	int16_t pError;
	//DCL_HANDLE gpioh_moto_sleep, gpioh_power_en;
	
	//StopTimer(ALLOY_LOCK_TIMER);
	kal_cancel_timer(motor_sleep_timer);
    dbg_printf("motor_driver_sleep: runing state %d", motor_runing_state);
    get_lock_int_state();
    
	if(motor_runing_state == MOTOR_ACTION_NONE)
	{
	    if(lock_internal_state == LOCK_INT_STATE_LOCKED) {
    		//EINT_AckInt(LOCKER_SENSE_LOCK_EINT_NO);
    		EINT_Mask(LOCKER_SENSE_LOCK_EINT_NO);
            //EINT_AckInt(LOCKER_SENSE_UNLOCK_EINT_NO);
    		EINT_Mask(LOCKER_SENSE_UNLOCK_EINT_NO);
    		//gpioh_moto_sleep = DclGPIO_Open(DCL_GPIO, gpio_motodrv_sleep);
    		//gpioh_power_en = DclGPIO_Open(DCL_GPIO, gpio_moto_power_en);
    		
    		//DclGPIO_Control(gpioh_moto_sleep, GPIO_CMD_WRITE_LOW, NULL);
    		GPIO_WriteIO(0, GPIO_MOTORDRV_NSLEEP);
    		//DclGPIO_Control(gpioh_power_en, GPIO_CMD_WRITE_LOW, NULL);

            // turn off motor power
    		GPIO_WriteIO(0, GPIO_MOTO_POWER_EN);
    		//DclGPIO_Close(gpioh_moto_sleep);
    		//DclGPIO_Close(gpioh_power_en);
        } else {
            lock_unlock(DIR_LOCK);
        }
	}
	else
	{
		//StartTimer(ALLOY_LOCK_TIMER, 500, (FuncPtr)motor_driver_sleep);
		kal_set_timer(motor_sleep_timer, (kal_timer_func_ptr)motor_driver_sleep, NULL, LOCKER_SLEEP_TIME*1000/4615, 0);
		return;
	}

}

static void motor_stop()
{
    //DCL_HANDLE gpioh_power_en, gpioh_moto_en, gpioh_moto_ph, gpioh_moto_in1, gpioh_moto_in2, gpioh_moto_sleep;

#ifdef MOTOR_DRV8837
	
	//gpioh_moto_in1=DclGPIO_Open(DCL_GPIO, gpio_motodrv_in1);
	//gpioh_moto_in2=DclGPIO_Open(DCL_GPIO, gpio_motodrv_in2);
	//gpioh_moto_sleep=DclGPIO_Open(DCL_GPIO, gpio_motodrv_sleep);

	// brake motor
	//DclGPIO_Control(gpioh_moto_in1, GPIO_CMD_WRITE_HIGH, NULL);
	//DclGPIO_Control(gpioh_moto_in2, GPIO_CMD_WRITE_HIGH, NULL);
	GPIO_WriteIO(1, GPIO_MOTORDRV_IN1);
	GPIO_WriteIO(1, GPIO_MOTORDRV_IN2);

	// delay for brake
	//delayms(200);
	dbg_printf("motor_stop after runing %d ms", drv_get_duration_ms(t1));

	//DclGPIO_Close(gpioh_moto_in1);
	//DclGPIO_Close(gpioh_moto_in2);
	//DclGPIO_Close(gpioh_moto_sleep);
	
#else //MOTOR_DRV8838
#endif
    if(lock_internal_state == LOCK_INT_STATE_LOCKED) {
    	//StartTimer(ALLOY_LOCK_TIMER, 100, (FuncPtr)motor_driver_sleep);
    	kal_set_timer(motor_sleep_timer, (kal_timer_func_ptr)motor_driver_sleep, NULL, LOCKER_SLEEP_TIME*1000/4615, 0);
    } else {
        kal_set_timer(motor_sleep_timer, (kal_timer_func_ptr)motor_driver_sleep, NULL, LOCKER_AUTO_LOCK_TIME*1000/4615, 0);
    }
	motor_runing_state=0;
}

static void lock_unlock_callback(void *para)
{
	kal_uint8 result;

	get_lock_int_state();

	if(para)
	{
		// called from protect timer
		if((*(kal_uint32 *)para) == motor_protect_flag)
		{
			// something wrong whith locker sensing switch
            kal_cancel_timer(motor_timer);
            if(motor_runing_state == MOTOR_ACTION_LOCK)
                locker_failed_count++;
            else if(motor_runing_state == MOTOR_ACTION_UNLOCK)
                locker_failed_count += 0x10000;
            locker_failed_count = locker_failed_count&0xFFF0FFF;
			dbg_printf("motor protect timer timeout! action: %d cur_state: %d failed_count: %x\n",
                motor_runing_state, lock_internal_state, locker_failed_count);
            motor_stop();
            return;
		}
	}

    check_motor_position();

    if(lock_internal_state == LOCK_INT_STATE_INTERMEDIATE)
        return;

	kal_cancel_timer(motor_timer);
	motor_stop();

	if(lock_internal_state == LOCK_INT_STATE_CONFLICT)
		dbg_printf("lock_internal_state conflict!");
    if((motor_runing_state==MOTOR_ACTION_LOCK && lock_internal_state!=LOCK_INT_STATE_LOCKED) || 
          (motor_runing_state==MOTOR_ACTION_UNLOCK && lock_internal_state!=LOCK_INT_STATE_UNLOCKED)){
	    dbg_printf("lock_unlock_callback: failed to %s ! cur state: %d",
	        motor_runing_state==MOTOR_ACTION_LOCK?"lock":"unlock", lock_internal_state);
    } else {
        // succeed
        locker_failed_count = 0;
    }
}

static void LOCKER_STATE_HISR(uint8_t sw)
{
	kal_uint8 open_sw = get_unlock_sense_switch_state();
    kal_uint8 close_sw = get_lock_sense_switch_state();
	EINT_Set_Polarity(LOCKER_SENSE_UNLOCK_EINT_NO, open_sw);
    EINT_Set_Polarity(LOCKER_SENSE_LOCK_EINT_NO, close_sw);
    //EINT_AckInt(LOCKER_SENSE_UNLOCK_EINT_NO);
    //EINT_AckInt(LOCKER_SENSE_LOCK_EINT_NO);
	dbg_printf("LOCKER_STATE_HISR(%d)(%d)(%d)", sw, open_sw, close_sw);

    if(motor_runing_state)
    {
        //get_lock_int_state();
#if (LOCKER_POST_WORKING_TIME == 0)
        lock_unlock_callback(NULL);
#else
        kal_cancel_timer(motor_timer);
        kal_set_timer(motor_timer, (kal_timer_func_ptr)lock_unlock_callback, NULL, LOCKER_POST_WORKING_TIME*1000/4615, 0);
#endif
    }
}

static void LOCKER_OPEN_STATE_HISR(void)
{
    if(locker_initialized)
        LOCKER_STATE_HISR(EINT_FLAG_OPEN_SW);
}

static void LOCKER_CLOSE_STATE_HISR(void)
{
    if(locker_initialized)
        LOCKER_STATE_HISR(EINT_FLAG_CLOSE_SW);
}

kal_uint8 locker_get_state()
{
	uint32_t int_st;
    int_st = get_lock_int_state();

    dbg_printf("locker_get_state %d", int_st);

	if(int_st == LOCK_INT_STATE_LOCKED)
		return LOCK_STATE_LOCKED;
	else if(int_st == LOCK_INT_STATE_UNLOCKED)
		return LOCK_STATE_UNLOCKED;
    else
        return LOCK_STATE_UNKOWN;
}

kal_bool lock_unlock(kal_uint8 lock)
{
    dbg_printf("lock_unlock: %d\n", lock);
    get_lock_int_state();
    if((lock == DIR_LOCK && lock_internal_state == LOCK_INT_STATE_LOCKED)
            || (lock == DIR_UNLOCK && lock_internal_state == LOCK_INT_STATE_UNLOCKED)) {
        dbg_printf("lock_unlock: already in %s state\n",
            lock_internal_state==LOCK_INT_STATE_LOCKED?"locked":"unlocked");
        return true;
    }

    if(motor_runing_state != MOTOR_ACTION_NONE) {
        dbg_printf("lock_unlock: motor is busy runing(%d)\n", motor_runing_state);
        return true;
    }

    if(get_lock_failed_count(locker_failed_count) > LOCKER_FAILED_TH
        || get_unlock_failed_count(locker_failed_count) > LOCKER_FAILED_TH) {
        dbg_printf("locker failed count exceed threshold(%x)\n", locker_failed_count);
        return true;
    }

    // use kal_timer
    kal_set_timer(motor_timer, (kal_timer_func_ptr)lock_unlock_callback, (void *)&motor_protect_flag, LOCKER_PROTECT_TIME*1000/4615, 0);
    if(lock == DIR_LOCK)
    {
    	motor_start(DIR_LOCK);
    }
    else if(lock == DIR_UNLOCK)
    {
    	motor_start(DIR_UNLOCK);
    }
    return true;
}

void locker_init()
{
    if(locker_initialized)
        return;

	motor_timer = kal_create_timer("MOTOR TIMER");
	motor_sleep_timer = kal_create_timer("MOTOR SLEEP TIMER");

	//EINT1 for locker sensing
	GPIO_ModeSetup(GPIO_LOCKER_SENSE_UNLOCK_PIN, 2/*EINT*/);
	GPIO_InitIO(0/*input*/, GPIO_LOCKER_SENSE_UNLOCK_PIN);
    GPIO_ModeSetup(GPIO_LOCKER_SENSE_LOCK_PIN, 2/*EINT*/);
	GPIO_InitIO(0/*input*/, GPIO_LOCKER_SENSE_LOCK_PIN);

    GPIO_ModeSetup(GPIO_MOTO_POWER_EN, 0);
	GPIO_InitIO(1/*output*/, GPIO_MOTO_POWER_EN);	
    // turn on when motor_start
	GPIO_WriteIO(0, GPIO_MOTO_POWER_EN);
	//delayms(5);
	
	//EINT_Set_HW_Debounce(LOCKER_SENSE_UNLOCK_EINT_NO, 100);
	EINT_Registration(LOCKER_SENSE_UNLOCK_EINT_NO, 
        KAL_TRUE, get_unlock_sense_switch_state(), LOCKER_OPEN_STATE_HISR, KAL_TRUE);
	EINT_Mask(LOCKER_SENSE_UNLOCK_EINT_NO);
	EINT_Set_Sensitivity(LOCKER_SENSE_UNLOCK_EINT_NO, 0/*LEVEL_SENSITIVE*/);

	//EINT_Set_HW_Debounce(LOCKER_SENSE_LOCK_EINT_NO, 100);
    EINT_Registration(LOCKER_SENSE_LOCK_EINT_NO, 
        KAL_TRUE, get_lock_sense_switch_state(), LOCKER_CLOSE_STATE_HISR, KAL_TRUE);
	EINT_Mask(LOCKER_SENSE_LOCK_EINT_NO);
	EINT_Set_Sensitivity(LOCKER_SENSE_LOCK_EINT_NO, 0/*LEVEL_SENSITIVE*/);

    locker_initialized = true;

	get_lock_int_state();
	//dbg_printf("locker_init lock_internal_state: %d", lock_internal_state);
    check_motor_position();

    if(lock_internal_state != LOCK_INT_STATE_LOCKED) {
        lock_unlock(DIR_LOCK);
    }
}

