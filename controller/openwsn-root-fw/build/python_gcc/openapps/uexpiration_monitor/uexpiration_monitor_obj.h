/**
DO NOT EDIT DIRECTLY!!

This file was 'objectified' by SCons as a pre-processing
step for the building a Python extension module.

This was done on 2018-03-08 18:27:34.483701.
*/
#ifndef __UMONITOR_H
#define __UMONITOR_H

/**
\addtogroup AppUdp
\{
\addtogroup umonitor
\{
*/

#include "Python.h"

#include "openudp_obj.h"

//=========================== define ==========================================

//=========================== typedef =========================================

//=========================== variables =======================================

typedef struct {
   udp_resource_desc_t desc;  ///< resource descriptor for this module, used to register at UDP stack
} umonitor_vars_t;

#include "openwsnmodule_obj.h"
typedef struct OpenMote OpenMote;

//=========================== prototypes ======================================

void umonitor_init(OpenMote* self);
void umonitor_receive(OpenMote* self, OpenQueueEntry_t* msg);
void umonitor_sendDone(OpenMote* self, OpenQueueEntry_t* msg, owerror_t error);
bool umonitor_debugPrint(OpenMote* self);

/**
\}
\}
*/

#endif
