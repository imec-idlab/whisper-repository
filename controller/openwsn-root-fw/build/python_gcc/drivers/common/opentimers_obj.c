/**
DO NOT EDIT DIRECTLY!!

This file was 'objectified' by SCons as a pre-processing
step for the building a Python extension module.

This was done on 2018-03-08 18:28:21.479566.
*/
/**
\brief Definition of the "opentimers" driver.

This driver uses a single hardware timer, which it virtualizes to support
at most MAX_NUM_TIMERS timers.

\author Tengfei Chang <tengfei.chang@inria.fr>, April 2017.
 */

#include "opendefs_obj.h"
#include "opentimers_obj.h"
#include "sctimer_obj.h"
#include "leds_obj.h"

//=========================== define ==========================================

//=========================== variables =======================================

// declaration of global variable _opentimers_vars_ removed during objectification.

//=========================== prototypes ======================================

void opentimers_timer_callback(OpenMote* self);

//=========================== public ==========================================

/**
\brief Initialize this module.

Initializes data structures and hardware timer.
 */
void opentimers_init(OpenMote* self){
    uint8_t i;
    // initialize local variables
    memset(&(self->opentimers_vars),0,sizeof(opentimers_vars_t));
    for (i=0;i<MAX_NUM_TIMERS;i++){
        // by default, all timers have the priority of 0xff (lowest priority)
        (self->opentimers_vars).timersBuf[i].priority = 0xff;
    }
    // set callback for sctimer module
 sctimer_set_callback(self, opentimers_timer_callback);
} 

/**
\brief create a timer by assigning an entry from timer buffer.

create a timer by assigning an Id for the timer.

\returns the id of the timer will be returned
 */
opentimers_id_t opentimers_create(OpenMote* self){
    uint8_t id;
    for (id=0;id<MAX_NUM_TIMERS;id++){
        if ((self->opentimers_vars).timersBuf[id].isUsed  == FALSE){
            (self->opentimers_vars).timersBuf[id].isUsed   = TRUE;
            return id;
        }
    }
    // there is no available buffer for this timer
    return TOO_MANY_TIMERS_ERROR;
}

/**
\brief schedule a period refer to comparing value set last time.

This function will schedule a timer which expires when the timer count reach 
to current counter + duration.

\param[in] id indicates the timer id
\param[in] duration indicates the period asked for schedule since last comparing value
\param[in] uint_type indicates the unit type of this schedule: ticks or ms
\param[in] timer_type indicates the timer type of this schedule: oneshot or periodic
\param[in] cb indicates when this scheduled timer fired, call this callback function.
 */
void opentimers_scheduleIn(OpenMote* self, opentimers_id_t    id, 
                           uint32_t           duration,
                           time_type_t        uint_type, 
                           timer_type_t       timer_type, 
                           opentimers_cbt     cb){
    uint8_t  i;
    uint8_t  idToSchedule;
    PORT_TIMER_WIDTH timerGap;
    PORT_TIMER_WIDTH tempTimerGap;
    // 1. make sure the timer exist
    for (i=0;i<MAX_NUM_TIMERS;i++){
        if ((self->opentimers_vars).timersBuf[i].isUsed && i == id){
            break;
        }
    }
    if (i==MAX_NUM_TIMERS){
        // doesn't find the timer
        return;
    }
    
    (self->opentimers_vars).timersBuf[id].timerType = timer_type;
    
    // 2. updat the timer content
    switch (uint_type){
    case TIME_MS:
        (self->opentimers_vars).timersBuf[id].totalTimerPeriod = duration*PORT_TICS_PER_MS;
        (self->opentimers_vars).timersBuf[id].wraps_remaining  = (uint32_t)(duration*PORT_TICS_PER_MS)/MAX_TICKS_IN_SINGLE_CLOCK;
        break;
    case TIME_TICS:
        (self->opentimers_vars).timersBuf[id].totalTimerPeriod = duration;
        (self->opentimers_vars).timersBuf[id].wraps_remaining  = (uint32_t)(duration)/MAX_TICKS_IN_SINGLE_CLOCK;
        break;
    }
    
    if ((self->opentimers_vars).timersBuf[id].wraps_remaining==0){
        (self->opentimers_vars).timersBuf[id].currentCompareValue = (self->opentimers_vars).timersBuf[id].totalTimerPeriod+ sctimer_readCounter(self);
    } else {
        (self->opentimers_vars).timersBuf[id].currentCompareValue = MAX_TICKS_IN_SINGLE_CLOCK+ sctimer_readCounter(self);
    }
    
    (self->opentimers_vars).timersBuf[id].isrunning           = TRUE;
    (self->opentimers_vars).timersBuf[id].callback            = cb;
    
    // 3. find the next timer to fire
    
    // only execute update the currenttimeout if I am not inside of ISR or the ISR itself will do this.
    if ((self->opentimers_vars).insideISR==FALSE){
        timerGap     = (self->opentimers_vars).timersBuf[0].currentCompareValue-(self->opentimers_vars).lastTimeout;
        idToSchedule = 0;
        for (i=1;i<MAX_NUM_TIMERS;i++){
            if ((self->opentimers_vars).timersBuf[i].isrunning){
                tempTimerGap = (self->opentimers_vars).timersBuf[i].currentCompareValue-(self->opentimers_vars).lastTimeout;
                if (tempTimerGap < timerGap){
                    // if a timer "i" has low priority but has compare value less than 
                    // candidate timer "idToSchedule" more than TIMERTHRESHOLD ticks, 
                    // replace candidate timer by this timer "i".
                    if ((self->opentimers_vars).timersBuf[i].priority > (self->opentimers_vars).timersBuf[idToSchedule].priority){
                        if (timerGap-tempTimerGap > TIMERTHRESHOLD){
                            timerGap     = tempTimerGap;
                            idToSchedule = i;
                        }
                    } else {
                        // a timer "i" has higher priority than candidate timer "idToSchedule" 
                        // and compare value less than candidate timer replace candidate 
                        // timer by timer "i".
                        timerGap     = tempTimerGap;
                        idToSchedule = i;
                    }
                } else {
                    // if a timer "i" has higher priority than candidate timer "idToSchedule" 
                    // and its compare value is larger than timer "i" no more than TIMERTHRESHOLD ticks,
                    // replace candidate timer by timer "i".
                    if ((self->opentimers_vars).timersBuf[i].priority < (self->opentimers_vars).timersBuf[idToSchedule].priority){
                        if (tempTimerGap - timerGap < TIMERTHRESHOLD){
                            timerGap     = tempTimerGap;
                            idToSchedule = i;
                        }
                    }
                }
            }
        }
        
        // if I got here, assign the next to be fired timer to given timer
        (self->opentimers_vars).currentTimeout = (self->opentimers_vars).timersBuf[idToSchedule].currentCompareValue;
 sctimer_setCompare(self, (self->opentimers_vars).currentTimeout);
    }
    (self->opentimers_vars).running        = TRUE;
}

/**
\brief schedule a period refer to given reference.

This function will schedule a timer which expires when the timer count reach 
to duration + reference. This function will be used in the implementation of slot FSM.
All timers use this function are ONE_SHOT type timer.

\param[in] id indicates the timer id
\param[in] duration indicates the period asked for schedule after a given time indicated by reference parameter.
\param[in] uint_type indicates the unit type of this schedule: ticks or ms
\param[in] cb indicates when this scheduled timer fired, call this callback function.
 */
void opentimers_scheduleAbsolute(OpenMote* self, opentimers_id_t    id, 
                                 uint32_t           duration, 
                                 PORT_TIMER_WIDTH   reference , 
                                 time_type_t        uint_type, 
                                 opentimers_cbt     cb){
    uint8_t  i;
    uint8_t idToSchedule;
    PORT_TIMER_WIDTH timerGap;
    PORT_TIMER_WIDTH tempTimerGap;
    // 1. make sure the timer exist
    for (i=0;i<MAX_NUM_TIMERS;i++){
        if ((self->opentimers_vars).timersBuf[i].isUsed && i == id){
            break;
        }
    }
    if (i==MAX_NUM_TIMERS){
        // doesn't find the timer
        return;
    }
    
    // absolute scheduling is for one shot timer
    (self->opentimers_vars).timersBuf[id].timerType = TIMER_ONESHOT;
    
    // 2. updat the timer content
    switch (uint_type){
    case TIME_MS:
        (self->opentimers_vars).timersBuf[id].totalTimerPeriod = duration*PORT_TICS_PER_MS;
        (self->opentimers_vars).timersBuf[id].wraps_remaining  = (uint32_t)(duration*PORT_TICS_PER_MS)/MAX_TICKS_IN_SINGLE_CLOCK;;
        break;
    case TIME_TICS:
        (self->opentimers_vars).timersBuf[id].totalTimerPeriod = duration;
        (self->opentimers_vars).timersBuf[id].wraps_remaining  = (uint32_t)duration/MAX_TICKS_IN_SINGLE_CLOCK;
        break;
    }
    
    if ((self->opentimers_vars).timersBuf[id].wraps_remaining==0){
        (self->opentimers_vars).timersBuf[id].currentCompareValue = (self->opentimers_vars).timersBuf[id].totalTimerPeriod+reference;
    } else {
        (self->opentimers_vars).timersBuf[id].currentCompareValue = MAX_TICKS_IN_SINGLE_CLOCK+reference;
    }

    (self->opentimers_vars).timersBuf[id].isrunning           = TRUE;
    (self->opentimers_vars).timersBuf[id].callback            = cb;
    
    // 3. find the next timer to fire
    
    // only execute update the currenttimeout if I am not inside of ISR or the ISR itself will do this.
    if ((self->opentimers_vars).insideISR==FALSE){
        timerGap     = (self->opentimers_vars).timersBuf[0].currentCompareValue-(self->opentimers_vars).lastTimeout;
        idToSchedule = 0;
        for (i=1;i<MAX_NUM_TIMERS;i++){
            if ((self->opentimers_vars).timersBuf[i].isrunning){
                tempTimerGap = (self->opentimers_vars).timersBuf[i].currentCompareValue-(self->opentimers_vars).lastTimeout;
                if (tempTimerGap < timerGap){
                    // if a timer "i" has low priority but has compare value less than 
                    // candidate timer "idToSchedule" more than TIMERTHRESHOLD ticks, 
                    // replace candidate timer by this timer "i".
                    if ((self->opentimers_vars).timersBuf[i].priority > (self->opentimers_vars).timersBuf[idToSchedule].priority){
                        if (timerGap-tempTimerGap > TIMERTHRESHOLD){
                            timerGap     = tempTimerGap;
                            idToSchedule = i;
                        }
                    } else {
                        // a timer "i" has higher priority than candidate timer "idToSchedule" 
                        // and compare value less than candidate timer replace candidate 
                        // timer by timer "i".
                        timerGap     = tempTimerGap;
                        idToSchedule = i;
                    }
                } else {
                    // if a timer "i" has higher priority than candidate timer "idToSchedule" 
                    // and its compare value is larger than timer "i" no more than TIMERTHRESHOLD ticks,
                    // replace candidate timer by timer "i".
                    if ((self->opentimers_vars).timersBuf[i].priority < (self->opentimers_vars).timersBuf[idToSchedule].priority){
                        if (tempTimerGap - timerGap < TIMERTHRESHOLD){
                            timerGap     = tempTimerGap;
                            idToSchedule = i;
                        }
                    }
                }
            }
        }
        
        // if I got here, assign the next to be fired timer to given timer
        (self->opentimers_vars).currentTimeout = (self->opentimers_vars).timersBuf[idToSchedule].currentCompareValue;
 sctimer_setCompare(self, (self->opentimers_vars).currentTimeout);
    }
    (self->opentimers_vars).running        = TRUE;
}

/**
\brief cancel a running timer.

This function disable the timer temperally by removing its callback and marking
isrunning as false. The timer may be recover later.

\param[in] id the timer id
 */
void opentimers_cancel(OpenMote* self, opentimers_id_t id){
    (self->opentimers_vars).timersBuf[id].isrunning = FALSE;
    (self->opentimers_vars).timersBuf[id].callback  = NULL;
}

/**
\brief destroy a stored timer.

Reset the whole entry of given timer including the id.

\param[in] id the timer id

\returns False if the given can't be found or return Success
 */
bool opentimers_destroy(OpenMote* self, opentimers_id_t id){
    if (id<MAX_NUM_TIMERS){
        memset(&(self->opentimers_vars).timersBuf[id],0,sizeof(opentimers_t));
        return TRUE;
    } else {
        return FALSE;
    }
}

/**
\brief get the current counter value of sctimer.

\returns the current counter value.
 */
PORT_TIMER_WIDTH opentimers_getValue(OpenMote* self){
    return sctimer_readCounter(self);
}

/**
\brief get the currentTimeout variable of opentimer2.

\returns currentTimeout.
 */
PORT_TIMER_WIDTH opentimers_getCurrentTimeout(OpenMote* self){
    return (self->opentimers_vars).currentTimeout;
}

/**
\brief is the given timer running?

\returns isRunning variable.
 */
bool opentimers_isRunning(OpenMote* self, opentimers_id_t id){
    return (self->opentimers_vars).timersBuf[id].isrunning;
}


/**
\brief set the priority of given timer

\param[in] id indicates the timer to be assigned.
\param[in] priority indicates the priority of given timer.
 */
void opentimers_setPriority(OpenMote* self, opentimers_id_t id, uint8_t priority){
    if ((self->opentimers_vars).timersBuf[id].isUsed  == TRUE){
        (self->opentimers_vars).timersBuf[id].priority = priority;
    } else {
        // the given timer is not used, do nothing.
    }
}

// ========================== callback ========================================

/**
\brief this is the callback function of opentimer2.

This function is called when sctimer interrupt happens. The function looks the 
whole timer buffer and find out the correct timer responding to the interrupt
and call the callback recorded for that timer.
 */
void opentimers_timer_callback(OpenMote* self){
    uint8_t i;
    uint8_t j;
    uint8_t idToCallCB;
    uint8_t idToSchedule;
    PORT_TIMER_WIDTH timerGap;
    PORT_TIMER_WIDTH tempTimerGap;
    PORT_TIMER_WIDTH tempLastTimeout = (self->opentimers_vars).currentTimeout;
    // 1. find the expired timer
    for (i=0;i<MAX_NUM_TIMERS;i++){
        if ((self->opentimers_vars).timersBuf[i].isrunning==TRUE){
            // all timers in the past within TIMERTHRESHOLD ticks
            // (probably with low priority) will mared as Expired.
            if ((self->opentimers_vars).currentTimeout-(self->opentimers_vars).timersBuf[i].currentCompareValue <= TIMERTHRESHOLD){
                // this timer expired, mark as expired
                (self->opentimers_vars).timersBuf[i].hasExpired = TRUE;
                // find the fired timer who has the smallest currentTimeout as last Timeout
                if (tempLastTimeout>(self->opentimers_vars).timersBuf[i].currentCompareValue){
                    tempLastTimeout = (self->opentimers_vars).timersBuf[i].currentCompareValue;
                }
            }
        }
    }
    
    // update lastTimeout
    (self->opentimers_vars).lastTimeout                               = tempLastTimeout;
    
    // 2. call the callback of expired timers
    (self->opentimers_vars).insideISR = TRUE;
    
    idToCallCB = TOO_MANY_TIMERS_ERROR;
    // find out the timer expired with highest priority 
    for (j=0;j<MAX_NUM_TIMERS;j++){
        if ((self->opentimers_vars).timersBuf[j].hasExpired == TRUE){
            if (idToCallCB==TOO_MANY_TIMERS_ERROR){
                idToCallCB = j;
            } else {
                if ((self->opentimers_vars).timersBuf[j].priority<(self->opentimers_vars).timersBuf[idToCallCB].priority){
                    idToCallCB = j;
                }
            }
        }
    }
    if (idToCallCB==TOO_MANY_TIMERS_ERROR){
        // no more timer expired
    } else {
        // call all timers expired having the same priority with timer idToCallCB
        for (j=0;j<MAX_NUM_TIMERS;j++){
            if (
                (self->opentimers_vars).timersBuf[j].hasExpired == TRUE &&
                (self->opentimers_vars).timersBuf[j].priority   == (self->opentimers_vars).timersBuf[idToCallCB].priority
            ){
                (self->opentimers_vars).timersBuf[j].lastCompareValue    = (self->opentimers_vars).timersBuf[j].currentCompareValue;
                if ((self->opentimers_vars).timersBuf[j].wraps_remaining==0){
                    (self->opentimers_vars).timersBuf[j].isrunning           = FALSE;
                    (self->opentimers_vars).timersBuf[j].hasExpired          = FALSE;
                    (self->opentimers_vars).timersBuf[j].callback(self, j);
                    if ((self->opentimers_vars).timersBuf[j].timerType==TIMER_PERIODIC){
 opentimers_scheduleIn(self, j,
                                              (self->opentimers_vars).timersBuf[j].totalTimerPeriod,
                                              TIME_TICS,
                                              TIMER_PERIODIC,
                                              (self->opentimers_vars).timersBuf[j].callback
                        );
                    }
                } else {
                    (self->opentimers_vars).timersBuf[j].wraps_remaining--;
                    if ((self->opentimers_vars).timersBuf[j].wraps_remaining == 0){
                        (self->opentimers_vars).timersBuf[j].currentCompareValue = ((self->opentimers_vars).timersBuf[j].totalTimerPeriod+(self->opentimers_vars).timersBuf[j].lastCompareValue) & MAX_TICKS_IN_SINGLE_CLOCK;
                    } else {
                        (self->opentimers_vars).timersBuf[j].currentCompareValue = (self->opentimers_vars).timersBuf[j].lastCompareValue + MAX_TICKS_IN_SINGLE_CLOCK;
                    }
                    (self->opentimers_vars).timersBuf[j].hasExpired          = FALSE;
                }
            }
        }
    }
    (self->opentimers_vars).insideISR = FALSE;
      
    // 3. find the next timer to be fired
    timerGap     = (self->opentimers_vars).timersBuf[0].currentCompareValue+(self->opentimers_vars).timersBuf[0].wraps_remaining*MAX_TICKS_IN_SINGLE_CLOCK-(self->opentimers_vars).lastTimeout;
    idToSchedule = 0;
    for (i=1;i<MAX_NUM_TIMERS;i++){
        if ((self->opentimers_vars).timersBuf[i].isrunning){
            tempTimerGap = (self->opentimers_vars).timersBuf[i].currentCompareValue-(self->opentimers_vars).lastTimeout;
            if (tempTimerGap < timerGap){
                // if a timer "i" has low priority but has compare value less than 
                // candidate timer "idToSchedule" more than TIMERTHRESHOLD ticks, 
                // replace candidate timer by this timer "i".
                if ((self->opentimers_vars).timersBuf[i].priority > (self->opentimers_vars).timersBuf[idToSchedule].priority){
                    if (timerGap-tempTimerGap > TIMERTHRESHOLD){
                        timerGap     = tempTimerGap;
                        idToSchedule = i;
                    }
                } else {
                    // a timer "i" has higher priority than candidate timer "idToSchedule" 
                    // and compare value less than candidate timer replace candidate 
                    // timer by timer "i".
                    timerGap     = tempTimerGap;
                    idToSchedule = i;
                }
            } else {
                // if a timer "i" has higher priority than candidate timer "idToSchedule" 
                // and its compare value is larger than timer "i" no more than TIMERTHRESHOLD ticks,
                // replace candidate timer by timer "i".
                if ((self->opentimers_vars).timersBuf[i].priority < (self->opentimers_vars).timersBuf[idToSchedule].priority){
                    if (tempTimerGap - timerGap < TIMERTHRESHOLD){
                        timerGap     = tempTimerGap;
                        idToSchedule = i;
                    }
                }
            }
        }
    }
    
    // 4. reschedule the timer
    (self->opentimers_vars).currentTimeout = (self->opentimers_vars).timersBuf[idToSchedule].currentCompareValue;
    (self->opentimers_vars).lastCompare[(self->opentimers_vars).index] = (self->opentimers_vars).currentTimeout;
    (self->opentimers_vars).index = ((self->opentimers_vars).index+1)&0x0F;
 sctimer_setCompare(self, (self->opentimers_vars).currentTimeout);
    (self->opentimers_vars).running        = TRUE;
}