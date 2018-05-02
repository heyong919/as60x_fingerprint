#ifndef _LOCKER_H_
#define _LOCKER_H_


#define LOCK_STATE_LOCKED       0
#define LOCK_STATE_UNLOCKED     1
#define LOCK_STATE_UNKOWN       2

#define DIR_LOCK		        1
#define DIR_UNLOCK		        0

// internal state
#define LOCK_INT_STATE_UNLOCKED         LOCK_STATE_UNLOCKED
#define LOCK_INT_STATE_INTERMEDIATE     4
#define LOCK_INT_STATE_LOCKED           LOCK_STATE_LOCKED
#define LOCK_INT_STATE_CONFLICT         8

#define LOCKER_PROTECT_TIME             2000    // millisecond, max moving time for protection
#define LOCKER_POST_WORKING_TIME        0 //150
#define LOCKER_SLEEP_TIME               100//ms
#define LOCKER_AUTO_LOCK_TIME           2000//ms

#define MOTOR_ACTION_NONE       0
#define MOTOR_ACTION_LOCK       1
#define MOTOR_ACTION_UNLOCK     2

#define UNKNOWN_POS             0
#define LOCK_POS                1
#define UNLOCK_POS              2
#define INTERMEDIATE_POS        3

void locker_init();
kal_bool lock_unlock(kal_uint8 lock);
kal_uint8 locker_get_state();

#endif
