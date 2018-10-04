/**
DO NOT EDIT DIRECTLY!!

This file was 'objectified' by SCons as a pre-processing
step for the building a Python extension module.

This was done on 2018-03-08 18:26:29.405823.
*/
#ifndef __DEBUGPINS_H
#define __DEBUGPINS_H

/**
\addtogroup BSP
\{
\addtogroup debugpins
\{

\brief Cross-platform declaration "leds" bsp module.

\author Thomas Watteyne <watteyne@eecs.berkeley.edu>, February 2012.
*/

//=========================== define ==========================================

//=========================== typedef =========================================

//=========================== variables =======================================

#include "openwsnmodule_obj.h"
typedef struct OpenMote OpenMote;

//=========================== prototypes ======================================

void debugpins_init(OpenMote* self);

void debugpins_frame_toggle(OpenMote* self);
void debugpins_frame_clr(OpenMote* self);
void debugpins_frame_set(OpenMote* self);

void debugpins_slot_toggle(OpenMote* self);
void debugpins_slot_clr(OpenMote* self);
void debugpins_slot_set(OpenMote* self);

void debugpins_fsm_toggle(OpenMote* self);
void debugpins_fsm_clr(OpenMote* self);
void debugpins_fsm_set(OpenMote* self);

void debugpins_task_toggle(OpenMote* self);
void debugpins_task_clr(OpenMote* self);
void debugpins_task_set(OpenMote* self);

void debugpins_isr_toggle(OpenMote* self);
void debugpins_isr_clr(OpenMote* self);
void debugpins_isr_set(OpenMote* self);

void debugpins_radio_toggle(OpenMote* self);
void debugpins_radio_clr(OpenMote* self);
void debugpins_radio_set(OpenMote* self);

#ifdef OPENSIM
void debugpins_ka_clr(OpenMote* self);
void debugpins_ka_set(OpenMote* self);

void debugpins_syncPacket_clr(OpenMote* self);
void debugpins_syncPacket_set(OpenMote* self);

void debugpins_syncAck_clr(OpenMote* self);
void debugpins_syncAck_set(OpenMote* self);

void debugpins_debug_clr(OpenMote* self);
void debugpins_debug_set(OpenMote* self);
#endif

/**
\}
\}
*/

#endif
