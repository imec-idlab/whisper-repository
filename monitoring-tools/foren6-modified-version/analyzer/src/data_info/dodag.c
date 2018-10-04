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
 *         RPL DODAG Information Mangement
 * \author
 *         Foren6 Team <foren6@cetic.be>
 *         http://cetic.github.io/foren6/credits.html
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "dodag.h"
#include "node.h"
#include "rpl_data.h"
#include "../data_collector/rpl_event_callbacks.h"

struct di_dodag {
    di_dodag_key_t key;         //Via DIO & DAO for dodagid and via DIO for version

    //Configuration
    rpl_instance_config_t instance_config;      //Via DIO config option
    rpl_instance_config_delta_t instance_config_delta;

    bool has_dodag_config;
    rpl_dodag_config_t dodag_config;    //Via DIO config option
    rpl_dodag_config_delta_t dodag_config_delta;

    bool has_prefix_info;
    rpl_prefix_t prefix_info;   //Via DIO prefix option
    rpl_prefix_delta_t prefix_info_delta;

    di_rpl_instance_ref_t rpl_instance; //Via DIO, DAO

    //Nodes
    hash_container_ptr nodes;   //Via DIO, sometimes DAO

    bool has_changed;
    void *user_data;
};

static void dodag_set_changed(di_dodag_t * dodag);

size_t
dodag_sizeof()
{
    return sizeof(di_dodag_t);
}

void
dodag_init(void *data, const void *key, size_t key_size)
{
    di_dodag_t *dodag = (di_dodag_t *) data;

    assert(key_size == sizeof(di_dodag_ref_t));

    dodag->nodes = hash_create(sizeof(di_node_ref_t), NULL);
    dodag->rpl_instance.rpl_instance = -1;
    dodag->key.ref = *(di_dodag_ref_t *) key;
    memset(&dodag->instance_config, 0, sizeof(dodag->instance_config));
    memset(&dodag->instance_config_delta, 0,
           sizeof(dodag->instance_config_delta));
    dodag->has_dodag_config = false;
    memset(&dodag->dodag_config, 0, sizeof(dodag->dodag_config));
    memset(&dodag->dodag_config_delta, 0, sizeof(dodag->dodag_config_delta));
    dodag->has_prefix_info = false;
    memset(&dodag->prefix_info, 0, sizeof(dodag->prefix_info));
    memset(&dodag->prefix_info_delta, 0, sizeof(dodag->prefix_info_delta));
    dodag->has_changed = true;
    rpl_event_dodag(dodag, RET_Created);
}

void
dodag_destroy(void *data)
{
    di_dodag_t *dodag = (di_dodag_t *) data;

    hash_destroy(dodag->nodes);
}

di_dodag_t *
dodag_dup(const di_dodag_t * dodag)
{
    di_dodag_t *new_dodag;

    new_dodag = malloc(sizeof(di_dodag_t));
    memcpy(new_dodag, dodag, sizeof(di_dodag_t));
    new_dodag->nodes = hash_dup(dodag->nodes);

    return new_dodag;
}

void
dodag_key_init(di_dodag_key_t * key, addr_ipv6_t dodag_id,
               uint8_t dodag_version, uint32_t version)
{
    memset(key, 0, sizeof(di_dodag_key_t));

    key->ref.dodagid = dodag_id;
    key->ref.version = dodag_version;
}

void
dodag_ref_init(di_dodag_ref_t * ref, addr_ipv6_t dodag_id,
               uint8_t dodag_version)
{
    memset(ref, 0, sizeof(di_dodag_ref_t));

    ref->dodagid = dodag_id;
    ref->version = dodag_version;
}

void
dodag_set_key(di_dodag_t * dodag, const di_dodag_key_t * key)
{
    if(memcmp(&dodag->key, key, sizeof(di_dodag_key_t))) {
        dodag->key = *key;
        dodag_set_changed(dodag);
    }
}

const di_dodag_key_t *
dodag_get_key(const di_dodag_t * dodag)
{
    return &dodag->key;
}

void
dodag_set_user_data(di_dodag_t * dodag, void *user_data)
{
    dodag->user_data = user_data;
}

void *
dodag_get_user_data(const di_dodag_t * dodag)
{
    return dodag->user_data;
}

static void
dodag_set_changed(di_dodag_t * dodag)
{
    if(dodag->has_changed == false)
        rpl_event_dodag(dodag, RET_Updated);
    dodag->has_changed = true;
}

bool
dodag_has_changed(di_dodag_t * dodag)
{
    return dodag->has_changed;
}

void
dodag_reset_changed(di_dodag_t * dodag)
{
    dodag->has_changed = false;
}


void
dodag_set_nodes_changed(di_dodag_t * dodag)
{
    hash_iterator_ptr it, itend;

    it = hash_begin(dodag->nodes, NULL);
    itend = hash_end(dodag->nodes, NULL);

    for(; hash_it_equ(it, itend) == false; hash_it_inc(it)) {
        di_node_t *node =
            rpldata_get_node(hash_it_value(it), HVM_FailIfNonExistant, NULL);
        assert(node != NULL);
        assert(!memcmp
               (node_get_dodag(node), &dodag->key.ref,
                sizeof(di_dodag_ref_t)));
        node_update_from_dodag(node, dodag);
    }

    hash_it_destroy(it);
    hash_it_destroy(itend);
}

void
dodag_update_from_dio(di_dodag_t * dodag, const rpl_dio_t * dio)
{
    if(!dio)
        return;
    rpl_instance_config_t new_config = dodag->instance_config;

    update_rpl_instance_config_from_dio(&new_config, dio);
    if(rpl_instance_config_compare(&new_config, &dodag->instance_config)) {
        dodag->instance_config = new_config;
        //dodag_set_nodes_changed(dodag);
        dodag_set_changed(dodag);
    }
}

void
dodag_update_from_dodag_config(di_dodag_t * dodag,
                               const rpl_dodag_config_t * dodag_config)
{
    if(dodag_config) {
        if(rpl_dodag_config_compare(dodag_config, &dodag->dodag_config)) {
            dodag->dodag_config = *dodag_config;
            //dodag_set_nodes_changed(dodag);
            dodag_set_changed(dodag);
        }
        dodag->has_dodag_config = true;
    } else {
        if(dodag->has_dodag_config) {
            //dodag_set_nodes_changed(dodag);
            dodag_set_changed(dodag);
        }
        dodag->has_dodag_config = false;
    }
}

void
dodag_update_from_dodag_prefix_info(di_dodag_t * dodag,
                                    const rpl_prefix_t * prefix)
{
    if(prefix) {
        if(rpl_prefix_compare(prefix, &dodag->prefix_info)) {
            dodag->prefix_info = *prefix;
            //dodag_set_nodes_changed(dodag);
            dodag_set_changed(dodag);
        }
        dodag->has_prefix_info = true;
    } else {
        if(dodag->has_prefix_info) {
            //dodag_set_nodes_changed(dodag);
            dodag_set_changed(dodag);
        }
        dodag->has_prefix_info = false;
    }
}

void
dodag_set_rpl_instance(di_dodag_t * dodag,
                       const di_rpl_instance_ref_t * rpl_instance)
{
    if(dodag->rpl_instance.rpl_instance != rpl_instance->rpl_instance) {
        dodag->rpl_instance = *rpl_instance;
        dodag_set_changed(dodag);
    }
}

void
dodag_add_node(di_dodag_t * dodag, di_node_t * node)
{
    bool was_already_in_dodag = false;
    const di_dodag_ref_t *previous_dodag_ref = node_get_dodag(node);

    if(previous_dodag_ref && previous_dodag_ref->version >= 0 &&
       (previous_dodag_ref->version != dodag->key.ref.version
        || memcmp(&previous_dodag_ref->dodagid, &dodag->key.ref.dodagid,
                  sizeof(addr_ipv6_t)))
        ) {
        di_dodag_t *previous_dodag;

        previous_dodag =
            rpldata_get_dodag(previous_dodag_ref, HVM_FailIfNonExistant,
                              NULL);
        if(previous_dodag == NULL) {
            fprintf(stderr, "Node old dodag does not exist\n");
        } else {
            dodag_del_node(previous_dodag, node);
        }
    }

    hash_add(dodag->nodes, hash_key_make(node_get_key(node)->ref),
             &node_get_key(node)->ref, NULL, HAM_OverwriteIfExists,
             &was_already_in_dodag);

    if(was_already_in_dodag == false) {
        node_set_dodag(node, &dodag->key.ref);
        //node_update_ip(node, &dodag->prefix_info.prefix);
        dodag_set_changed(dodag);
    } else {
        assert(previous_dodag_ref->version == dodag->key.ref.version
               && !memcmp(&previous_dodag_ref->dodagid,
                          &dodag->key.ref.dodagid, sizeof(addr_ipv6_t)));
    }
}

void
dodag_del_node(di_dodag_t * dodag, di_node_t * node)
{
    static const di_dodag_ref_t null_ref = { {{{0}}}, -1 };

    if(hash_delete(dodag->nodes, hash_key_make(node_get_key(node)->ref))) {
        node_set_dodag(node, &null_ref);
        dodag_set_changed(dodag);
    }
}

const rpl_instance_config_t *
dodag_get_instance_config(const di_dodag_t * dodag)
{
    return &dodag->instance_config;
}

const rpl_dodag_config_t *
dodag_get_dodag_config(const di_dodag_t * dodag)
{
    if(dodag->has_dodag_config) {
        return &dodag->dodag_config;
    } else {
        return NULL;
    }
}

const rpl_prefix_t *
dodag_get_prefix(const di_dodag_t * dodag)
{
    if(dodag->has_prefix_info) {
        return &dodag->prefix_info;
    } else {
        return NULL;
    }
}

const di_rpl_instance_ref_t *
dodag_get_rpl_instance(const di_dodag_t * dodag)
{
    return &dodag->rpl_instance;
}

hash_container_ptr
dodag_get_node(const di_dodag_t * dodag)
{
    return dodag->nodes;
}
