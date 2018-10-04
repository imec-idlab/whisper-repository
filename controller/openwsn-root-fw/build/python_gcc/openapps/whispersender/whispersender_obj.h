/**
DO NOT EDIT DIRECTLY!!

This file was 'objectified' by SCons as a pre-processing
step for the building a Python extension module.

This was done on 2018-03-08 18:27:34.924277.
*/
#ifndef __WHISPERSENDER_H
#define __WHISPERSENDER_H

/**
\addtogroup AppUdp
\{
\addtogroup whispersender
\{
*/

#include "Python.h"

#include "opentimers_obj.h"
#include "openudp_obj.h"
#include "opencoap_obj.h"

//=========================== define ==========================================

#define WHISPERSENDER_PERIOD_MS 1000

//=========================== typedef =========================================

//=========================== variables =======================================

typedef struct {
opentimers_id_t     timerIdWhispersender;
   uint16_t             counter;  ///< incrementing counter which is written into the packet
   uint16_t              period;  ///< whispersender packet sending period>
   coap_resource_desc_t descoap;
   udp_resource_desc_t descudp;
} whispersender_vars_t;

#include "openwsnmodule_obj.h"
typedef struct OpenMote OpenMote;

//=========================== prototypes ======================================

void whispersender_init(OpenMote* self);
owerror_t whispersender_receiveCoap(OpenMote* self, 
      OpenQueueEntry_t* msg,
      coap_header_iht* coap_header,
      coap_option_iht* coap_options
   );
void whispersender_receiveUdp(OpenMote* self, OpenQueueEntry_t* pkt);
void whispersender_sendDoneCoap(OpenMote* self, OpenQueueEntry_t* msg, owerror_t error);
void whispersender_sendDoneUdp(OpenMote* self, OpenQueueEntry_t* msg, owerror_t error);
void whispersender_timer_cb(OpenMote* self, opentimers_id_t id);
void whispersender_task_cb(OpenMote* self);
/**
\}
\}
*/

#endif

