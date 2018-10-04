#ifndef __WHISPERSENDER_H
#define __WHISPERSENDER_H

/**
\addtogroup AppUdp
\{
\addtogroup whispersender
\{
*/

#include "opentimers.h"
#include "openudp.h"
#include "opencoap.h"

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

//=========================== prototypes ======================================

void whispersender_init(void);
owerror_t whispersender_receiveCoap(
      OpenQueueEntry_t* msg,
      coap_header_iht* coap_header,
      coap_option_iht* coap_options
   );
void whispersender_receiveUdp(OpenQueueEntry_t* pkt);
void whispersender_sendDoneCoap(OpenQueueEntry_t* msg, owerror_t error);
void whispersender_sendDoneUdp(OpenQueueEntry_t* msg, owerror_t error);
void whispersender_timer_cb(opentimers_id_t id);
void whispersender_task_cb(void);
/**
\}
\}
*/

#endif

