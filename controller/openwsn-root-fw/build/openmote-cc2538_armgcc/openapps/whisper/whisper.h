#ifndef __WHISPER_H
#define __WHISPER_H

/**
\addtogroup AppCoAP
\{
\addtogroup whisper
\{
*/

#include "opencoap.h"
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

//=========================== prototypes ======================================



void whisper_init(void);

void whisper_setState(uint8_t i);
uint8_t whisper_getState(void);
uint8_t whisper_getTargetChildren(void);
uint8_t whisper_getTargetParentOld(void);
uint8_t whisper_getTargetParentNew(void);
void whisper_setTargetParentOld(uint8_t i);
void whisper_setTargetParentNew(uint8_t i);
void whisper_setTargetChildren(uint8_t i);
void whisper_setFakeSource(uint8_t i);
uint8_t whisper_getFakeSource(void);

void whisper_timer_cb(opentimers_id_t id);
void whisper_task_cb(void);

void whisper_timer_cb2(opentimers_id_t id);
void whisper_task_cb2(void);

void whisper_timer_cb3(opentimers_id_t id);
void whisper_task_cb4(void);

void whisper_timer_cb4(opentimers_id_t id);
void whisper_task_cb4(void);

void whisper_trigger_nextStep(void);


void whisper_task_remote(uint8_t* buf, uint8_t bufLen);

//uint8_t whisper_getSeqNumOfNeigh(uint8_t n);
//void whisper_setFakeSource(uint8_t n,uint8_t i);

owerror_t whisper_receive(
      OpenQueueEntry_t* msg,
      coap_header_iht* coap_header,
      coap_option_iht* coap_options
   );
void whisper_sendDone(OpenQueueEntry_t* msg, owerror_t error);
/**
\}
\}
*/

#endif
