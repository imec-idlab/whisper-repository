/**
DO NOT EDIT DIRECTLY!!

This file was 'objectified' by SCons as a pre-processing
step for the building a Python extension module.

This was done on 2018-03-08 18:25:59.359490.
*/
#ifndef __UECHO_H
#define __UECHO_H

/**
\addtogroup AppUdp
\{
\addtogroup uecho
\{
*/

#include "Python.h"

#include "openudp_obj.h"

//=========================== define ==========================================

//=========================== typedef =========================================

//=========================== variables =======================================

typedef struct {
   udp_resource_desc_t desc;  ///< resource descriptor for this module, used to register at UDP stack
} uecho_vars_t;

#include "openwsnmodule_obj.h"
typedef struct OpenMote OpenMote;

//=========================== prototypes ======================================

void uecho_init(OpenMote* self);
void uecho_receive(OpenMote* self, OpenQueueEntry_t* msg);
void uecho_sendDone(OpenMote* self, OpenQueueEntry_t* msg, owerror_t error);
bool uecho_debugPrint(OpenMote* self);

/**
\}
\}
*/

#endif
