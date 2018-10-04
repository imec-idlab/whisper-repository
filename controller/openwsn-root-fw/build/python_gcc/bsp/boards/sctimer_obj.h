/**
DO NOT EDIT DIRECTLY!!

This file was 'objectified' by SCons as a pre-processing
step for the building a Python extension module.

This was done on 2018-03-08 18:26:30.667618.
*/
#ifndef __SCTIMER_H
#define __SCTIMER_H

/**
\addtogroup BSP
\{
\addtogroup sctimer
\{

\brief A timer module with only a single compare value.

\author Tengfei Chang <tengfei.chang@inria.fr>, April 2017.
*/

#include "Python.h"

#include "stdint.h"
#include "board_obj.h"

//=========================== typedef =========================================

typedef void  (*sctimer_cbt)(OpenMote* self);
typedef void  (*sctimer_capture_cbt)(OpenMote* self, PORT_TIMER_WIDTH timestamp);

//=========================== variables =======================================


#include "openwsnmodule_obj.h"
typedef struct OpenMote OpenMote;

//=========================== prototypes ======================================

void sctimer_init(OpenMote* self);
void sctimer_setCompare(OpenMote* self, PORT_TIMER_WIDTH val);
void sctimer_set_callback(OpenMote* self, sctimer_cbt cb);
void     sctimer_setStartFrameCb(sctimer_capture_cbt cb);
void     sctimer_setEndFrameCb(sctimer_capture_cbt cb);
PORT_TIMER_WIDTH sctimer_readCounter(OpenMote* self);
void sctimer_enable(OpenMote* self);
void sctimer_disable(OpenMote* self);

kick_scheduler_t sctimer_isr(void);

/**
\}
\}
*/

#endif
