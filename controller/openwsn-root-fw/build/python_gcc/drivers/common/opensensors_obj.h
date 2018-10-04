/**
DO NOT EDIT DIRECTLY!!

This file was 'objectified' by SCons as a pre-processing
step for the building a Python extension module.

This was done on 2018-03-08 18:28:15.670529.
*/
/**
    \brief Declaration of the "opensensors" driver.
    \author Nicola Accettura <nicola.accettura@eecs.berkeley.edu>, March 2015.
*/

#ifndef __OPENSENSORS_H__
#define __OPENSENSORS_H__


/**
\addtogroup drivers
\{
\addtogroup OpenSensors
\{
*/

#include "Python.h"

#include "sensors.h"

//=========================== define ==========================================

//=========================== typedef =========================================

typedef struct {
   uint8_t                          sensorType;
   callbackRead_cbt                 callbackRead;
   callbackConvert_cbt              callbackConvert;
} opensensors_resource_desc_t;

//=========================== module variables ================================

typedef struct {
   opensensors_resource_desc_t      opensensors_resource[NUMSENSORS];
   uint8_t                          numSensors;
} opensensors_vars_t;

#include "openwsnmodule_obj.h"
typedef struct OpenMote OpenMote;

//=========================== prototypes ======================================

void opensensors_init(void);
uint8_t opensensors_getNumSensors(void);
opensensors_resource_desc_t* opensensors_getResource(
    uint8_t index
);

/**
\}
\}
*/

#endif // __OPENSENSORS_H__
