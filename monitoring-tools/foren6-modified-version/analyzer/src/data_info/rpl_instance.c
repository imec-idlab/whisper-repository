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
 *         RPL Instances
 * \author
 *         Foren6 Team <foren6@cetic.be>
 *         http://cetic.github.io/foren6/credits.html
 */

#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "rpl_instance.h"
#include "dodag.h"
#include "../data_collector/rpl_event_callbacks.h"

struct di_rpl_instance {
    di_rpl_instance_key_t key;

    hash_container_ptr dodags;  //Via DIO, DAO
    di_rpl_mop_e mode_of_operation;     //Via DIO

    bool has_changed;
    void *user_data;
};

static void rpl_instance_set_changed(di_rpl_instance_t * rpl_instance);

size_t
rpl_instance_sizeof()
{
    return sizeof(di_rpl_instance_t);
}

void
rpl_instance_init(void *data, const void *key, size_t key_size)
{
    di_rpl_instance_t *instance = (di_rpl_instance_t *) data;

    assert(key_size == sizeof(di_rpl_instance_ref_t));

    instance->dodags = hash_create(sizeof(di_dodag_ref_t), NULL);
    instance->key.ref = *(di_rpl_instance_ref_t *) key;
    instance->has_changed = true;
    rpl_event_rpl_instance(instance, RET_Created);
}

void
rpl_instance_destroy(void *data)
{
    di_rpl_instance_t *instance = (di_rpl_instance_t *) data;

    hash_destroy(instance->dodags);
}

di_rpl_instance_t *
rpl_instance_dup(const di_rpl_instance_t * rpl_instance)
{
    di_rpl_instance_t *new_instance;

    new_instance = malloc(sizeof(di_rpl_instance_t));
    memcpy(new_instance, rpl_instance, sizeof(di_rpl_instance_t));
    new_instance->dodags = hash_dup(rpl_instance->dodags);

    return new_instance;
}

void
rpl_instance_key_init(di_rpl_instance_key_t * key, uint8_t rpl_instance,
                      uint32_t version)
{
    memset(key, 0, sizeof(di_rpl_instance_key_t));

    key->ref.rpl_instance = rpl_instance;
}

void
rpl_instance_ref_init(di_rpl_instance_ref_t * ref, uint8_t rpl_instance)
{
    memset(ref, 0, sizeof(di_rpl_instance_ref_t));

    ref->rpl_instance = rpl_instance;
}

void
rpl_instance_set_key(di_rpl_instance_t * rpl_instance,
                     const di_rpl_instance_key_t * key)
{
    if(memcmp(&rpl_instance->key, key, sizeof(di_rpl_instance_key_t))) {
        rpl_instance->key = *key;
        rpl_instance_set_changed(rpl_instance);
    }
}

void
rpl_instance_set_mop(di_rpl_instance_t * rpl_instance, di_rpl_mop_e mop)
{
    if(rpl_instance->mode_of_operation != mop) {
        rpl_instance->mode_of_operation = mop;
        rpl_instance_set_changed(rpl_instance);
    }
}

void
rpl_instance_set_user_data(di_rpl_instance_t * rpl_instance, void *user_data)
{
    rpl_instance->user_data = user_data;
}

void
rpl_instance_add_dodag(di_rpl_instance_t * rpl_instance, di_dodag_t * dodag)
{
    bool was_already_existing;

    hash_add(rpl_instance->dodags, hash_key_make(dodag_get_key(dodag)->ref),
             &dodag_get_key(dodag)->ref, NULL, HAM_OverwriteIfExists,
             &was_already_existing);

    if(!was_already_existing) {
        dodag_set_rpl_instance(dodag, &rpl_instance->key.ref);
        rpl_instance_set_changed(rpl_instance);
    }
}

void
rpl_instance_del_dodag(di_rpl_instance_t * rpl_instance, di_dodag_t * dodag)
{
    static const di_rpl_instance_ref_t null_rpl_instance = { -1 };

    if(hash_delete
       (rpl_instance->dodags, hash_key_make(dodag_get_key(dodag)->ref))) {
        dodag_set_rpl_instance(dodag, &null_rpl_instance);
        rpl_instance_set_changed(rpl_instance);
    }
}

static void
rpl_instance_set_changed(di_rpl_instance_t * rpl_instance)
{
    if(rpl_instance->has_changed == false)
        rpl_event_rpl_instance(rpl_instance, RET_Updated);
    rpl_instance->has_changed = true;
}

bool
rpl_instance_has_changed(di_rpl_instance_t * rpl_instance)
{
    return rpl_instance->has_changed;
}

void
rpl_instance_reset_changed(di_rpl_instance_t * rpl_instance)
{
    rpl_instance->has_changed = false;
}


const di_rpl_instance_key_t *
rpl_instance_get_key(const di_rpl_instance_t * rpl_instance)
{
    return &rpl_instance->key;
}

di_rpl_mop_e
rpl_instance_get_mop(const di_rpl_instance_t * rpl_instance)
{
    return rpl_instance->mode_of_operation;
}

void *
rpl_instance_get_user_data(const di_rpl_instance_t * rpl_instance)
{
    return rpl_instance->user_data;
}
