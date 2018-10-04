/**
DO NOT EDIT DIRECTLY!!

This file was 'objectified' by SCons as a pre-processing
step for the building a Python extension module.

This was done on 2018-03-08 18:28:09.766731.
*/
#ifndef _CBORENCODER_H
#define _CBORENCODER_H

#include "Python.h"

#include "opendefs_obj.h"

uint8_t cborencoder_put_text(uint8_t **buffer, char *text, uint8_t text_len);

uint8_t cborencoder_put_array(uint8_t **buffer,uint8_t elements);

uint8_t cborencoder_put_bytes(uint8_t **buffer, uint8_t bytes_len, uint8_t *bytes);

uint8_t cborencoder_put_map(uint8_t **buffer, uint8_t elements);

uint8_t cborencoder_put_unsigned(uint8_t **buffer, uint8_t value);

#endif /* _CBORENCODER_H */
