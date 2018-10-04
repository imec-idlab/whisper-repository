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

#include <stdlib.h>
#include <string.h>

#include "circular_buffer.h"


static void circular_buffer_inc_write_idx(circular_buffer_t buf);
static void circular_buffer_inc_read_idx(circular_buffer_t buf);

circular_buffer_t
circular_buffer_create(int size, int element_size)
{
    circular_buffer_t new_buffer =
        (circular_buffer_t) malloc(sizeof(struct circular_buffer));

    if(!new_buffer)
        goto error;

    new_buffer->data = (uint8_t *) malloc(size * element_size);
    if(!new_buffer->data)
        goto error;

    new_buffer->size = size;
    new_buffer->element_size = element_size;
    new_buffer->read_idx = new_buffer->write_idx = 0;

    return new_buffer;

  error:
    if(new_buffer) {
        if(new_buffer->data)
            free(new_buffer->data);
        free(new_buffer);
    }

    return NULL;
}

void
circular_buffer_delete(circular_buffer_t buf)
{
    free(buf->data);
    free(buf);
}

bool
circular_buffer_is_empty(circular_buffer_t buf)
{
    return buf->read_idx == buf->write_idx;
}

bool
circular_buffer_is_full(circular_buffer_t buf)
{
    int next_index = buf->write_idx + 1;

    if(next_index >= buf->size)
        next_index = 0;

    return buf->read_idx == next_index;
}


bool
circular_buffer_push_front(circular_buffer_t buf, const void *data)
{
    if(circular_buffer_is_full(buf))
        return false;

    memcpy(&buf->data[buf->write_idx * buf->element_size], data,
           buf->element_size);

    circular_buffer_inc_write_idx(buf);

    return true;
}

void *
circular_buffer_pop_back(circular_buffer_t buf)
{
    void *data_ptr;

    if(circular_buffer_is_empty(buf))
        return NULL;

    data_ptr = &(buf->data[buf->read_idx * buf->element_size]);

    circular_buffer_inc_read_idx(buf);

    return data_ptr;
}

static void
circular_buffer_inc_write_idx(circular_buffer_t buf)
{
    int new_index = buf->write_idx + 1;

    if(new_index >= buf->size)
        buf->write_idx = 0;
    else
        buf->write_idx = new_index;
}


static void
circular_buffer_inc_read_idx(circular_buffer_t buf)
{
    int new_index = buf->read_idx + 1;

    if(new_index >= buf->size)
        buf->read_idx = 0;
    else
        buf->read_idx = new_index;
}
