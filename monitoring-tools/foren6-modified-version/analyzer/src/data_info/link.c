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
 *         Parent-Child Relationships in DODAG Management
 * \author
 *         Foren6 Team <foren6@cetic.be>
 *         http://cetic.github.io/foren6/credits.html
 */

#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include "link.h"
#include "../data_collector/rpl_event_callbacks.h"
#include "rpl_data.h"

struct di_link {
    di_link_key_t key;

    di_metric_t metric;
    time_t last_update;         //TX only
    time_t expiration_time;
    uint32_t packet_count;      //TX only

    bool deprecated;

    bool has_changed;
    void *user_data;
};

static void link_set_changed(di_link_t * link);
static void link_update_old_field(const di_link_t * link, int field_offset,
                                  int field_size);

size_t
link_sizeof()
{
    return sizeof(di_link_t);
}

void
link_init(void *data, const void *key, size_t key_size)
{
    di_link_t *link = (di_link_t *) data;

    assert(key_size == sizeof(di_link_ref_t));

    link->key.ref = *(di_link_ref_t *) key;
    link->has_changed = true;
    rpl_event_link(link, RET_Created);
}

void
link_destroy(void *data)
{
    data = data;                //prevent a unused arg warning
    // Nothing to do
}

void
link_key_init(di_link_key_t * key, di_node_ref_t child, di_node_ref_t parent,
              uint32_t version)
{
    memset(key, 0, sizeof(di_link_key_t));

    key->ref.child = child;
    key->ref.parent = parent;
}

void
link_ref_init(di_link_ref_t * ref, di_node_ref_t child, di_node_ref_t parent)
{
    memset(ref, 0, sizeof(di_link_ref_t));

    ref->child = child;
    ref->parent = parent;
}

bool
link_update(di_link_t * link, time_t time, uint32_t added_packet_count)
{
    link->last_update = time;
    link->packet_count += added_packet_count;
    link_update_old_field(link, offsetof(di_link_t, packet_count),
                          sizeof(link->packet_count));
    if(link->deprecated) {
        link->deprecated = false;
        link_set_changed(link);
    }
    return true;
}

static void
link_set_changed(di_link_t * link)
{
    if(link->has_changed == false)
        rpl_event_link(link, RET_Updated);
    link->has_changed = true;
}

static void
link_update_old_field(const di_link_t * link, int field_offset,
                      int field_size)
{
    di_link_t **versionned_link_ptr;
    int version = rpldata_get_wsn_last_version();

    if(version) {
        hash_container_ptr container = rpldata_get_links(version);

        if(container) {
            versionned_link_ptr =
                (di_link_t **) hash_value(container,
                                          hash_key_make(link->key.ref),
                                          HVM_FailIfNonExistant, NULL);
            if(versionned_link_ptr && *versionned_link_ptr != link)
                memcpy((char *)(*versionned_link_ptr) + field_offset,
                       (char *)link + field_offset, field_size);
        }
    }
}

di_link_t *
link_dup(const di_link_t * link)
{
    di_link_t *new_link;

    new_link = malloc(sizeof(di_link_t));
    memcpy(new_link, link, sizeof(di_link_t));

    return new_link;
}

void
link_set_key(di_link_t * link, di_link_key_t * key)
{
    if(memcmp(&link->key, key, sizeof(di_link_key_t))) {
        link->key = *key;
        link_set_changed(link);
    }
}

void
link_set_metric(di_link_t * link, const di_metric_t * metric)
{
    if(link->metric.type != metric->type
       || link->metric.value != metric->value) {
        link->metric = *metric;
        link_set_changed(link);
    }
}

void
link_set_user_data(di_link_t * link, void *user_data)
{
    link->user_data = user_data;
}

bool
link_has_changed(di_link_t * link)
{
    return link->has_changed;
}

void
link_reset_changed(di_link_t * link)
{
    link->has_changed = false;
}


const di_link_key_t *
link_get_key(const di_link_t * link)
{
    return &link->key;
}

time_t
link_get_last_update(const di_link_t * link)
{
    return link->last_update;
}

uint32_t
link_get_packet_count(const di_link_t * link)
{
    return link->packet_count;
}

const di_metric_t *
link_get_metric(const di_link_t * link)
{
    return &link->metric;
}

bool
link_get_deprecated(const di_link_t * link)
{
    return link->deprecated;
}

void *
link_get_user_data(const di_link_t * link)
{
    return link->user_data;
}

void
links_deprecate_all_from(di_link_ref_t const *new_link_ref)
{
    hash_iterator_ptr it = hash_begin(NULL, NULL);
    hash_iterator_ptr itEnd = hash_begin(NULL, NULL);
    hash_container_ptr working_container = rpldata_get_links(0);

    for(hash_begin(working_container, it), hash_end(working_container, itEnd);
        !hash_it_equ(it, itEnd); hash_it_inc(it)) {
        di_link_t **link_ptr = hash_it_value(it);
        di_link_t *link = (link_ptr) ? *link_ptr : NULL;
        di_link_ref_t link_ref = link_get_key(link)->ref;

        if(link_ref.child.wpan_address == new_link_ref->child.wpan_address &&
           link_ref.parent.wpan_address !=
           new_link_ref->parent.wpan_address) {
            link->deprecated = true;
            link_set_changed(link);
        }
    }

    hash_it_destroy(it);
    hash_it_destroy(itEnd);
}

void
links_clear_all_deprecated()
{
    hash_iterator_ptr it = hash_begin(NULL, NULL);
    hash_iterator_ptr itEnd = hash_begin(NULL, NULL);
    hash_container_ptr working_container = rpldata_get_links(0);
    bool removed;

    do {
        removed = false;
        for(hash_begin(working_container, it),
            hash_end(working_container, itEnd); !hash_it_equ(it, itEnd);
            hash_it_inc(it)) {
            di_link_t **link_ptr = hash_it_value(it);
            di_link_t *link = (link_ptr) ? *link_ptr : NULL;

            if(link->deprecated) {
                hash_it_delete_value(it);
                rpl_event_link(link, RET_Deleted);
                removed = true;
                break;
            }
        }
    } while(removed);

    hash_it_destroy(it);
    hash_it_destroy(itEnd);
}
