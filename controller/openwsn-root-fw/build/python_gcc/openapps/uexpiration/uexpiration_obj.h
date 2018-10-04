/**
DO NOT EDIT DIRECTLY!!

This file was 'objectified' by SCons as a pre-processing
step for the building a Python extension module.

This was done on 2018-03-08 18:27:34.691978.
*/
#ifndef __UEXPIRATION_H
#define __UEXPIRATION_H

/**
\addtogroup AppUdp
\{
\addtogroup uexpiration
\{
*/

#include "Python.h"

#include "opentimers_obj.h"
#include "openudp_obj.h"

//=========================== define ==========================================
//=========================== typedef =========================================

//=========================== variables =======================================

typedef struct {   
   opentimers_id_t        timerId;  ///< periodic timer which triggers transmission
   uint16_t               period;  ///< uinject packet sending period>
   udp_resource_desc_t    desc;  ///< resource descriptor for this module, used to register at UDP stack
} uexpiration_vars_t;

#include "openwsnmodule_obj.h"
typedef struct OpenMote OpenMote;

//=========================== prototypes ======================================

void uexpiration_init(OpenMote* self);
void uexpiration_receive(OpenMote* self, OpenQueueEntry_t* msg);
void uexpiration_sendDone(OpenMote* self, OpenQueueEntry_t* msg, owerror_t error);

/**
\}
\}
*/

#endif
