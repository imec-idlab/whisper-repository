/**
DO NOT EDIT DIRECTLY!!

This file was 'objectified' by SCons as a pre-processing
step for the building a Python extension module.

This was done on 2018-03-08 18:26:30.933920.
*/
#ifndef __WHISPER_H
#define __WHISPER_H

/**
\addtogroup AppCoAP
\{
\addtogroup whisper
\{
*/

#include "Python.h"

#include "opencoap_obj.h"
//=========================== define ==========================================

//=========================== typedef =========================================

//=========================== variables =======================================

typedef struct {
   coap_resource_desc_t desc;
//   opentimers_id_t     timerIdSixp;  ///< timer which triggers an event
//   opentimers_id_t     timerIdSixp2;  ///< timer which triggers an event
//   opentimers_id_t     timerIdSixp3;  ///< timer which triggers an event
//   opentimers_id_t     timerIdRpl;  ///< timer which triggers an event
   opentimers_id_t     timerIdWhisper;

} whisper_vars_t;

#include "openwsnmodule_obj.h"
typedef struct OpenMote OpenMote;

//=========================== prototypes ======================================



void whisper_init(OpenMote* self);

void whisper_setState(OpenMote* self, uint8_t i);
uint8_t whisper_getState(OpenMote* self);
uint8_t whisper_getTargetChildren(OpenMote* self);
uint8_t whisper_getTargetParentOld(OpenMote* self);
uint8_t whisper_getTargetParentNew(OpenMote* self);
void whisper_setTargetParentOld(OpenMote* self, uint8_t i);
void whisper_setTargetParentNew(OpenMote* self, uint8_t i);
void whisper_setTargetChildren(OpenMote* self, uint8_t i);
void whisper_setFakeSource(OpenMote* self, uint8_t i);
uint8_t whisper_getFakeSource(OpenMote* self);

void whisper_timer_cb(OpenMote* self, opentimers_id_t id);
void whisper_task_cb(OpenMote* self);

void whisper_timer_cb2(OpenMote* self, opentimers_id_t id);
void whisper_task_cb2(OpenMote* self);

void whisper_timer_cb3(OpenMote* self, opentimers_id_t id);
void whisper_task_cb4(OpenMote* self);

void whisper_timer_cb4(OpenMote* self, opentimers_id_t id);
void whisper_task_cb4(OpenMote* self);

void whisper_trigger_nextStep(OpenMote* self);


void whisper_task_remote(OpenMote* self, uint8_t* buf, uint8_t bufLen);

//uint8_t whisper_getSeqNumOfNeigh(uint8_t n);
//void whisper_setFakeSource(OpenMote* self, uint8_t n,uint8_t i);

owerror_t whisper_receive(OpenMote* self, 
      OpenQueueEntry_t* msg,
      coap_header_iht* coap_header,
      coap_option_iht* coap_options
   );
void whisper_sendDone(OpenMote* self, OpenQueueEntry_t* msg, owerror_t error);
/**
\}
\}
*/

#endif
