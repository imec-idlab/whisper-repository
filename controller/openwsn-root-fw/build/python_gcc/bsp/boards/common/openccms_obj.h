/**
DO NOT EDIT DIRECTLY!!

This file was 'objectified' by SCons as a pre-processing
step for the building a Python extension module.

This was done on 2018-03-08 18:28:25.439364.
*/
/**
\brief Definitions for AES CCMS implementation

\author Marcelo Barros de Almeida <marcelobarrosalmeida@gmail.com>, March 2015.
\author Malisa Vucinic <malishav@gmail.com>, June 2017.
*/
#ifndef __OPENCCMS_H__
#define __OPENCCMS_H__

#include "openwsnmodule_obj.h"
typedef struct OpenMote OpenMote;

//=========================== prototypes ======================================

owerror_t openccms_enc(OpenMote* self, uint8_t* a,uint8_t len_a,uint8_t* m,uint8_t* len_m,uint8_t* nonce, uint8_t l, uint8_t key[16], uint8_t len_mac);
owerror_t openccms_dec(OpenMote* self, uint8_t* a,uint8_t len_a,uint8_t* m,uint8_t* len_m,uint8_t* nonce, uint8_t l, uint8_t key[16], uint8_t len_mac);

#endif /* __OPENCCMS_H__ */
