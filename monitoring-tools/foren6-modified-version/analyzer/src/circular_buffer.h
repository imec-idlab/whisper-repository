/*
 * This file is part of Foren6, a 6LoWPAN Diagnosis Tool
 * Copyright (C) 2013, CETIC
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/**
 * \file
 *         Circular buffers for sniffer inputs
 * \author
 *         Foren6 Team <foren6@cetic.be>
 *         http://cetic.github.io/foren6/credits.html
 */

#ifndef CIRCULAR_BUFFER_H
#define	CIRCULAR_BUFFER_H

#include <stdbool.h>
#include <stdint.h>

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct circular_buffer {
    uint8_t *data;
    int size;
    int element_size;
    int read_idx;
    int write_idx;
} *circular_buffer_t;

circular_buffer_t circular_buffer_create(int size, int element_size);
void circular_buffer_delete(circular_buffer_t buf);
bool circular_buffer_is_empty(circular_buffer_t buf);
bool circular_buffer_is_full(circular_buffer_t buf);
bool circular_buffer_push_front(circular_buffer_t buf, const void *data);
void *circular_buffer_pop_back(circular_buffer_t buf);

#ifdef	__cplusplus
}
#endif
#endif                          /* CIRCULAR_BUFFER_H */
