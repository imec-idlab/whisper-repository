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
 *         RPL Data Access
 * \author
 *         Foren6 Team <foren6@cetic.be>
 *         http://cetic.github.io/foren6/credits.html
 */

#include <assert.h>

#include "rpl_data.h"
#include "../utlist.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "link.h"
#include "node.h"
#include "dodag.h"
#include "rpl_instance.h"
#include "../sniffer_packet_parser.h"
#include "../data_collector/rpl_event_callbacks.h"

typedef struct di_rpl_data {
    hash_container_ptr nodes;
    hash_container_ptr dodags;
    hash_container_ptr rpl_instances;
    hash_container_ptr links;
} di_rpl_data_t;

typedef struct di_rpl_wsn_state {
    uint32_t node_version;
    uint32_t dodag_version;
    uint32_t rpl_instance_version;
    uint32_t links_version;

    double timestamp;
    uint32_t packet_id;

    uint32_t has_errors;
} di_rpl_wsn_state_t;

typedef struct di_rpl_object_el {
    void *object;
    void (*destroy) (void *data);
    struct di_rpl_object_el *next;
    struct di_rpl_object_el *prev;
} di_rpl_object_el_t, *di_rpl_object_list_t;

typedef struct {
    di_rpl_object_list_t nodes;
    di_rpl_object_list_t dodags;
    di_rpl_object_list_t rpl_instances;
    di_rpl_object_list_t links;
} di_rpl_allocated_objects_t;


di_rpl_data_t collected_data;

uint32_t node_last_version = 0;
uint32_t dodag_last_version = 0;
uint32_t rpl_instance_last_version = 0;
uint32_t link_last_version = 0;

di_rpl_allocated_objects_t allocated_objects;

di_rpl_wsn_state_t *wsn_versions = 0;
uint32_t wsn_version_array_size = 0;
uint32_t wsn_last_version = 0;

uint32_t node_last_version_has_errors = 0;

void
rpldata_init()
{
    if(wsn_version_array_size < 256) {
        wsn_version_array_size = 256;
        wsn_versions =
            realloc(wsn_versions,
                    wsn_version_array_size * sizeof(di_rpl_wsn_state_t));
    }

    collected_data.nodes = hash_create(sizeof(hash_container_ptr), NULL);
    collected_data.dodags = hash_create(sizeof(hash_container_ptr), NULL);
    collected_data.rpl_instances =
        hash_create(sizeof(hash_container_ptr), NULL);
    collected_data.links = hash_create(sizeof(hash_container_ptr), NULL);

    uint32_t working_version = 0;
    hash_container_ptr hash_ptr;

    hash_ptr = hash_create(sizeof(di_node_t *), NULL);
    hash_add(collected_data.nodes, hash_key_make(working_version), &hash_ptr,
             NULL, HAM_NoCheck, NULL);

    hash_ptr = hash_create(sizeof(di_dodag_t *), NULL);
    hash_add(collected_data.dodags, hash_key_make(working_version), &hash_ptr,
             NULL, HAM_NoCheck, NULL);

    hash_ptr = hash_create(sizeof(di_rpl_instance_t *), NULL);
    hash_add(collected_data.rpl_instances, hash_key_make(working_version),
             &hash_ptr, NULL, HAM_NoCheck, NULL);

    hash_ptr = hash_create(sizeof(di_link_t *), NULL);
    hash_add(collected_data.links, hash_key_make(working_version), &hash_ptr,
             NULL, HAM_NoCheck, NULL);


    node_last_version = 0;
    dodag_last_version = 0;
    rpl_instance_last_version = 0;
    link_last_version = 0;
    wsn_last_version = 0;

    wsn_versions[0].node_version = 0;
    wsn_versions[0].dodag_version = 0;
    wsn_versions[0].rpl_instance_version = 0;
    wsn_versions[0].links_version = 0;
    wsn_versions[0].timestamp = 0;
    wsn_versions[0].has_errors = 0;
}

hash_container_ptr
rpldata_get_nodes(uint32_t version)
{
    if(wsn_versions[version].node_version < 0)
        return NULL;

    hash_key_t key = hash_key_make(wsn_versions[version].node_version);

    hash_container_ptr *ptr =
        hash_value(collected_data.nodes, key, HVM_FailIfNonExistant, NULL);
    if(ptr)
        return *ptr;
    else
        return NULL;
}

hash_container_ptr
rpldata_get_dodags(uint32_t version)
{
    if(wsn_versions[version].dodag_version < 0)
        return NULL;

    hash_container_ptr *ptr =
        hash_value(collected_data.dodags,
                   hash_key_make(wsn_versions[version].dodag_version),
                   HVM_FailIfNonExistant, NULL);
    if(ptr)
        return *ptr;
    else
        return NULL;
}

hash_container_ptr
rpldata_get_rpl_instances(uint32_t version)
{
    if(wsn_versions[version].rpl_instance_version < 0)
        return NULL;

    hash_container_ptr *ptr =
        hash_value(collected_data.rpl_instances,
                   hash_key_make(wsn_versions[version].rpl_instance_version),
                   HVM_FailIfNonExistant, NULL);
    if(ptr)
        return *ptr;
    else
        return NULL;
}

hash_container_ptr
rpldata_get_links(uint32_t version)
{
    if(wsn_versions[version].links_version < 0)
        return NULL;

    hash_container_ptr *ptr =
        hash_value(collected_data.links,
                   hash_key_make(wsn_versions[version].links_version),
                   HVM_FailIfNonExistant, NULL);
    if(ptr)
        return *ptr;
    else
        return NULL;
}


uint32_t
rpldata_add_node_version()
{
    hash_iterator_ptr it = hash_begin(NULL, NULL);
    hash_iterator_ptr itEnd = hash_begin(NULL, NULL);

    node_last_version++;
    uint32_t new_version = node_last_version;
    uint32_t old_version = new_version - 1;

    node_last_version_has_errors = 0;
    hash_container_ptr new_version_container =
        hash_create(sizeof(di_node_t *), NULL);
    hash_container_ptr working_container = rpldata_get_nodes(0);
    hash_container_ptr last_container =
        *(hash_container_ptr *) hash_value(collected_data.nodes,
                                           hash_key_make(old_version),
                                           HVM_FailIfNonExistant, NULL);

    for(hash_begin(working_container, it), hash_end(working_container, itEnd);
        !hash_it_equ(it, itEnd); hash_it_inc(it)) {
        di_node_t **node_ptr = hash_it_value(it);
        di_node_t *node = (node_ptr) ? *node_ptr : NULL;
        di_node_ref_t node_ref = node_get_key(node)->ref;
        di_node_t **last_node_ptr =
            (di_node_t **) hash_value(last_container, hash_key_make(node_ref),
                                      HVM_FailIfNonExistant, NULL);
        di_node_t *last_node = last_node_ptr ? *last_node_ptr : NULL;
        di_node_t *new_node;

        if(node_has_changed(node)) {
            node_reset_changed(node);
            new_node = node_dup(node);
            node_fill_delta(new_node, last_node);
            node_last_version_has_errors += node_get_has_errors(new_node);
        } else {
            new_node = last_node;
        }
        hash_add(new_version_container, hash_key_make(node_ref), &new_node,
                 NULL, HAM_NoCheck, NULL);
    }
    hash_add(collected_data.nodes, hash_key_make(new_version),
             &new_version_container, NULL, HAM_NoCheck, NULL);

    hash_it_destroy(it);
    hash_it_destroy(itEnd);

    return new_version;
}

uint32_t
rpldata_add_dodag_version()
{
    hash_iterator_ptr it = hash_begin(NULL, NULL);
    hash_iterator_ptr itEnd = hash_begin(NULL, NULL);

    dodag_last_version++;
    uint32_t new_version = dodag_last_version;
    uint32_t old_version = new_version - 1;

    hash_container_ptr new_version_container =
        hash_create(sizeof(di_dodag_t *), NULL);
    hash_container_ptr working_container = rpldata_get_dodags(0);
    hash_container_ptr last_container =
        *(hash_container_ptr *) hash_value(collected_data.dodags,
                                           hash_key_make(old_version),
                                           HVM_FailIfNonExistant, NULL);

    for(hash_begin(working_container, it), hash_end(working_container, itEnd);
        !hash_it_equ(it, itEnd); hash_it_inc(it)) {
        di_dodag_t **dodag_ptr = hash_it_value(it);
        di_dodag_t *dodag = (dodag_ptr) ? *dodag_ptr : NULL;
        di_dodag_ref_t dodag_ref = dodag_get_key(dodag)->ref;
        di_dodag_t *new_dodag;

        if(dodag_has_changed(dodag)) {
            dodag_reset_changed(dodag);
            new_dodag = dodag_dup(dodag);
        } else {
            new_dodag =
                *(di_dodag_t **) hash_value(last_container,
                                            hash_key_make(dodag_ref),
                                            HVM_FailIfNonExistant, NULL);
        }
        hash_add(new_version_container, hash_key_make(dodag_ref), &new_dodag,
                 NULL, HAM_NoCheck, NULL);
    }

    hash_add(collected_data.dodags, hash_key_make(new_version),
             &new_version_container, NULL, HAM_NoCheck, NULL);

    hash_it_destroy(it);
    hash_it_destroy(itEnd);

    return new_version;
}

uint32_t
rpldata_add_rpl_instance_version()
{
    hash_iterator_ptr it = hash_begin(NULL, NULL);
    hash_iterator_ptr itEnd = hash_begin(NULL, NULL);

    rpl_instance_last_version++;
    uint32_t new_version = rpl_instance_last_version;
    uint32_t old_version = new_version - 1;

    hash_container_ptr new_version_container =
        hash_create(sizeof(di_rpl_instance_t *), NULL);
    hash_container_ptr working_container = rpldata_get_rpl_instances(0);
    hash_container_ptr last_container =
        *(hash_container_ptr *) hash_value(collected_data.rpl_instances,
                                           hash_key_make(old_version),
                                           HVM_FailIfNonExistant, NULL);

    for(hash_begin(working_container, it), hash_end(working_container, itEnd);
        !hash_it_equ(it, itEnd); hash_it_inc(it)) {
        di_rpl_instance_t **rpl_instance_ptr = hash_it_value(it);
        di_rpl_instance_t *rpl_instance =
            (rpl_instance_ptr) ? *rpl_instance_ptr : NULL;
        di_rpl_instance_ref_t rpl_instance_ref =
            rpl_instance_get_key(rpl_instance)->ref;
        di_rpl_instance_t *new_rpl_instance;

        if(rpl_instance_has_changed(rpl_instance)) {
            rpl_instance_reset_changed(rpl_instance);
            new_rpl_instance = rpl_instance_dup(rpl_instance);
        } else
            new_rpl_instance =
                *(di_rpl_instance_t **) hash_value(last_container,
                                                   hash_key_make
                                                   (rpl_instance_ref),
                                                   HVM_FailIfNonExistant,
                                                   NULL);
        hash_add(new_version_container, hash_key_make(rpl_instance_ref),
                 &new_rpl_instance, NULL, HAM_NoCheck, NULL);
    }

    hash_add(collected_data.rpl_instances, hash_key_make(new_version),
             &new_version_container, NULL, HAM_NoCheck, NULL);

    hash_it_destroy(it);
    hash_it_destroy(itEnd);

    return new_version;
}

uint32_t
rpldata_add_link_version()
{
    hash_iterator_ptr it = hash_begin(NULL, NULL);
    hash_iterator_ptr itEnd = hash_begin(NULL, NULL);

    link_last_version++;
    uint32_t new_version = link_last_version;
    uint32_t old_version = new_version - 1;

    hash_container_ptr new_version_container =
        hash_create(sizeof(di_link_t *), NULL);
    hash_container_ptr working_container = rpldata_get_links(0);
    hash_container_ptr last_container =
        *(hash_container_ptr *) hash_value(collected_data.links,
                                           hash_key_make(old_version),
                                           HVM_FailIfNonExistant, NULL);

    for(hash_begin(working_container, it), hash_end(working_container, itEnd);
        !hash_it_equ(it, itEnd); hash_it_inc(it)) {
        di_link_t **link_ptr = hash_it_value(it);
        di_link_t *link = (link_ptr) ? *link_ptr : NULL;
        di_link_ref_t link_ref = link_get_key(link)->ref;
        di_link_t *new_link;

        if(link_has_changed(link)) {
            link_reset_changed(link);
            new_link = link_dup(link);
        } else
            new_link =
                *(di_link_t **) hash_value(last_container,
                                           hash_key_make(link_ref),
                                           HVM_FailIfNonExistant, NULL);

        hash_add(new_version_container, hash_key_make(link_ref), &new_link,
                 NULL, HAM_NoCheck, NULL);
    }

    hash_add(collected_data.links, hash_key_make(new_version),
             &new_version_container, NULL, HAM_NoCheck, NULL);

    hash_it_destroy(it);
    hash_it_destroy(itEnd);

    return new_version;
}

void *
rpldata_get_object(hash_container_ptr container, size_t object_size,
                   void (*onInit) (void *data, const void *key,
                                   size_t key_size),
                   void (*onDestroy) (void *data), hash_key_t key,
                   hash_value_mode_e value_mode, bool * was_created)
{
    bool already_existing;
    void *new_node;
    void **new_node_ptr;

    if(was_created == NULL)
        was_created = &already_existing;

    new_node_ptr =
        (void **)hash_value(container, key, HVM_FailIfNonExistant,
                            was_created);
    if(new_node_ptr)
        new_node = *new_node_ptr;
    else
        new_node = NULL;

    if(!new_node && value_mode == HVM_CreateIfNonExistant) {
        new_node = calloc(1, object_size);
        onInit(new_node, key.key, key.size);

        di_rpl_object_el_t *node_el = calloc(1, sizeof(di_rpl_object_el_t));

        node_el->object = new_node;
        node_el->destroy = onDestroy;

        DL_APPEND(allocated_objects.nodes, node_el);
        hash_add(container, key, &new_node, NULL, HAM_NoCheck, NULL);

        *was_created = true;
    } else
        *was_created = false;

    return new_node;
}

di_node_t *
rpldata_get_node(const di_node_ref_t * node_ref, hash_value_mode_e value_mode,
                 bool * was_created)
{
    return rpldata_get_object(rpldata_get_nodes(0), node_sizeof(), &node_init,
                              &node_destroy, hash_key_make(*node_ref),
                              value_mode, was_created);
}

di_dodag_t *
rpldata_get_dodag(const di_dodag_ref_t * dodag_ref,
                  hash_value_mode_e value_mode, bool * was_created)
{
    return rpldata_get_object(rpldata_get_dodags(0), dodag_sizeof(),
                              &dodag_init, &dodag_destroy,
                              hash_key_make(*dodag_ref), value_mode,
                              was_created);
}

di_rpl_instance_t *
rpldata_get_rpl_instance(const di_rpl_instance_ref_t * rpl_instance_ref,
                         hash_value_mode_e value_mode, bool * was_created)
{
    return rpldata_get_object(rpldata_get_rpl_instances(0),
                              rpl_instance_sizeof(), &rpl_instance_init,
                              &rpl_instance_destroy,
                              hash_key_make(*rpl_instance_ref), value_mode,
                              was_created);
}

di_link_t *
rpldata_get_link(const di_link_ref_t * link_ref, hash_value_mode_e value_mode,
                 bool * was_created)
{
    return rpldata_get_object(rpldata_get_links(0), link_sizeof(), &link_init,
                              &link_destroy, hash_key_make(*link_ref),
                              value_mode, was_created);
}

di_link_t *
rpldata_del_link(const di_link_ref_t * link_ref)
{
    bool found;
    di_link_t *deleted_link = NULL;
    hash_iterator_ptr it = hash_begin(NULL, NULL);

    found = hash_find(rpldata_get_links(0), hash_key_make(*link_ref), it);
    if(found) {
        deleted_link = *(di_link_t **) hash_it_value(it);
        hash_it_delete_value(it);
    }

    hash_it_destroy(it);

    return deleted_link;
}

void
rpldata_wsn_create_version(int packed_id, double timestamp)
{
    wsn_last_version++;

    if(wsn_version_array_size <= wsn_last_version) {
        wsn_version_array_size *= 2;
        wsn_versions =
            realloc(wsn_versions,
                    wsn_version_array_size * sizeof(di_rpl_wsn_state_t));
        assert(wsn_versions != NULL);
    }

    wsn_versions[wsn_last_version].timestamp = timestamp;
    wsn_versions[wsn_last_version].packet_id = packed_id;
    wsn_versions[wsn_last_version].has_errors = node_last_version_has_errors;
    node_last_version_has_errors = 0;

    if(node_last_version)
        wsn_versions[wsn_last_version].node_version = node_last_version;
    else
        wsn_versions[wsn_last_version].node_version = -1;

    if(dodag_last_version)
        wsn_versions[wsn_last_version].dodag_version = dodag_last_version;
    else
        wsn_versions[wsn_last_version].dodag_version = -1;

    if(rpl_instance_last_version)
        wsn_versions[wsn_last_version].rpl_instance_version =
            rpl_instance_last_version;
    else
        wsn_versions[wsn_last_version].rpl_instance_version = -1;

    if(link_last_version)
        wsn_versions[wsn_last_version].links_version = link_last_version;
    else
        wsn_versions[wsn_last_version].links_version = -1;

    rpl_event_process_events(wsn_last_version);
}

double
rpldata_wsn_version_get_timestamp(uint32_t version)
{
    if(version == 0)
        version = wsn_last_version;
    return wsn_versions[version].timestamp;
}

uint32_t
rpldata_wsn_version_get_packet_count(uint32_t version)
{
    if(version == 0)
        version = wsn_last_version;
    return wsn_versions[version].packet_id;
}

uint32_t
rpldata_wsn_version_get_has_errors(uint32_t version)
{
    if(version == 0)
        version = wsn_last_version;
    return wsn_versions[version].has_errors;
}

uint32_t
rpldata_get_node_last_version()
{
    return node_last_version;
}

uint32_t
rpldata_get_dodag_last_version()
{
    return dodag_last_version;
}

uint32_t
rpldata_get_rpl_instance_last_version()
{
    return rpl_instance_last_version;
}

uint32_t
rpldata_get_link_last_version()
{
    return link_last_version;
}

uint32_t
rpldata_get_wsn_last_version()
{
    return wsn_last_version;
}

void
rpldata_clear_objects(hash_container_ptr container,
                      void (*onDestroy) (void *data))
{
    hash_iterator_ptr itVersion, itVersionEnd, itObject, itObjectEnd;

    itVersion = hash_begin(NULL, NULL);
    itVersionEnd = hash_begin(NULL, NULL);
    itObject = hash_begin(NULL, NULL);
    itObjectEnd = hash_begin(NULL, NULL);

    for(hash_begin(container, itVersion), hash_end(container, itVersionEnd);
        !hash_it_equ(itVersion, itVersionEnd); hash_it_inc(itVersion)) {
        hash_container_ptr objects =
            *(hash_container_ptr *) hash_it_value(itVersion);

        for(hash_begin(objects, itObject), hash_end(objects, itObjectEnd);
            !hash_it_equ(itObject, itObjectEnd);) {
            hash_it_delete_value(itObject);
        }
        hash_destroy(objects);
    }
    hash_destroy(container);

    hash_it_destroy(itObjectEnd);
    hash_it_destroy(itObject);
    hash_it_destroy(itVersionEnd);
    hash_it_destroy(itVersion);
}

void
rpldata_clear()
{
    sniffer_parser_pause_parser(true);

    di_rpl_object_el_t *object_el, *tmp;

    DL_FOREACH_SAFE(allocated_objects.nodes, object_el, tmp) {
        object_el->destroy(object_el->object);
        DL_DELETE(allocated_objects.nodes, object_el);
        free(object_el->object);
        object_el->object = 0;
        free(object_el);
    }

    rpldata_clear_objects(collected_data.nodes, &node_destroy);
    rpldata_clear_objects(collected_data.dodags, &dodag_destroy);
    rpldata_clear_objects(collected_data.links, &link_destroy);
    rpldata_clear_objects(collected_data.rpl_instances,
                          &rpl_instance_destroy);

    rpldata_init();

    rpl_event_clear();

    sniffer_parser_pause_parser(false);
}
