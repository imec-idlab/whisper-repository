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
 *         Hash-based Information Management
 * \author
 *         Foren6 Team <foren6@cetic.be>
 *         http://cetic.github.io/foren6/credits.html
 */

#include <assert.h>

#include "hash_container.h"
#include "../uthash.h"
#include "pthread.h"
#include <stdio.h>
#ifdef __APPLE__
#include "pthread_spin_lock_shim.h"
#endif

typedef struct hash_container_el {
    void *data;
    void *key;
    size_t key_size;
    UT_hash_handle hh;
} hash_container_el_t;

struct hash_container {
    hash_container_el_t *head;
    size_t data_size;
    void (*data_constructor) (void *data, const void *key, size_t key_size);
    pthread_spinlock_t lock;
    int last_lock_line;
};

struct hash_iterator {
    hash_container_el_t *current_data;
    hash_container_el_t *next;
    hash_container_el_t *prev;
    hash_container_ptr container;
};

hash_container_ptr
hash_create(size_t data_size,
            void (*data_constructor) (void *data, const void *key,
                                      size_t key_size))
{
    hash_container_ptr new_container;

    new_container =
        (hash_container_ptr) calloc(1, sizeof(struct hash_container));
    new_container->data_size = data_size;
    new_container->data_constructor = data_constructor;
    pthread_spin_init(&new_container->lock, PTHREAD_PROCESS_PRIVATE);
    new_container->last_lock_line = 0;

    return new_container;
}

hash_container_ptr
hash_dup(const hash_container_ptr container)
{
    hash_container_ptr new_hash;
    struct hash_iterator it, end_it;

    new_hash = malloc(sizeof(struct hash_container));
    memcpy(new_hash, container, sizeof(struct hash_container));
    new_hash->head = NULL;
    pthread_spin_init(&new_hash->lock, PTHREAD_PROCESS_PRIVATE);
    new_hash->last_lock_line = 0;

    hash_begin(container, &it);
    hash_end(container, &end_it);

    for(; hash_it_equ(&it, &end_it) == false; hash_it_inc(&it)) {
        hash_add(new_hash, (hash_key_t) {
                 it.current_data->key, it.current_data->key_size}
                 , it.current_data->data, NULL, HAM_NoCheck, NULL);
    }

    return new_hash;
}

void
hash_destroy(hash_container_ptr container)
{
    hash_clear(container);
    free(container);
}

bool
hash_add(hash_container_ptr container, hash_key_t key, const void *data,
         hash_iterator_ptr iterator, hash_add_mode_e mode,
         bool * was_existing)
{
    void *data_content;

    data_content = calloc(1, container->data_size);
    if(data)
        memcpy(data_content, data, container->data_size);
    else if(container->data_constructor)
        container->data_constructor(data_content, key.key, key.size);

    return hash_add_ref(container, key, data_content, iterator, mode,
                        was_existing);
}

bool
hash_add_ref(hash_container_ptr container, hash_key_t key, const void *data,
             hash_iterator_ptr iterator, hash_add_mode_e mode,
             bool * was_existing)
{
    hash_container_el_t *element = NULL;

    assert(data != NULL);
    pthread_spin_lock(&container->lock);
    container->last_lock_line = __LINE__;

    if(mode != HAM_NoCheck) {
        HASH_FIND(hh, container->head, key.key, key.size, element);

        if(element && mode == HAM_FailIfExists) {
            if(iterator) {
                iterator->current_data = element;
                iterator->next = element->hh.next;
                iterator->prev = element->hh.prev;
                iterator->container = container;
            }
            if(was_existing)
                *was_existing = true;
            pthread_spin_unlock(&container->lock);
            return false;
        }
    }

    if(element == NULL) {
        element = calloc(1, sizeof(hash_container_el_t));
        element->data = (void *)data;
        element->key = malloc(key.size);
        element->key_size = key.size;
        memcpy(element->key, key.key, key.size);

        HASH_ADD_KEYPTR(hh, container->head, element->key, key.size, element);

        if(was_existing)
            *was_existing = false;
    } else {
        assert(!memcmp(element->key, key.key, key.size));
        element->data = (void *)data;

        if(was_existing)
            *was_existing = true;
    }

    if(iterator) {
        iterator->current_data = element;
        iterator->next = element->hh.next;
        iterator->prev = element->hh.prev;
        iterator->container = container;
    }

    pthread_spin_unlock(&container->lock);

    return true;
}

void *
hash_value(hash_container_ptr container, hash_key_t key,
           hash_value_mode_e mode, bool * was_created)
{
    bool found;
    struct hash_iterator iterator;

    found = hash_find(container, key, &iterator);

    if(!found && mode == HVM_CreateIfNonExistant) {
        if(was_created)
            *was_created = true;
        if(hash_add(container, key, NULL, &iterator, HAM_NoCheck, NULL) ==
           true) {
            return iterator.current_data->data;
        }
    } else if(iterator.current_data) {
        if(was_created)
            *was_created = false;
        return iterator.current_data->data;
    }

    if(was_created)
        *was_created = false;
    return NULL;
}

bool
hash_find(hash_container_ptr container, hash_key_t key,
          hash_iterator_ptr iterator)
{
    hash_container_el_t *element = NULL;


    pthread_spin_lock(&container->lock);
    container->last_lock_line = __LINE__;
    HASH_FIND(hh, container->head, key.key, key.size, element);
    pthread_spin_unlock(&container->lock);

    if(element) {
        if(iterator) {
            iterator->current_data = element;
            iterator->next = element->hh.next;
            iterator->prev = element->hh.prev;
            iterator->container = container;
        }

        return true;
    } else {
        if(iterator)
            hash_end(container, iterator);

        return false;
    }

}

void *
hash_remove_ref(hash_container_ptr container, hash_key_t key)
{
    bool found;
    void *data_ptr;
    struct hash_iterator iterator;

    data_ptr = NULL;

    found = hash_find(container, key, &iterator);
    if(found) {
        hash_it_remove_ref(&iterator);
    }

    return data_ptr;
}

bool
hash_delete(hash_container_ptr container, hash_key_t key)
{
    bool found;
    struct hash_iterator iterator;

    found = hash_find(container, key, &iterator);
    if(found) {
        hash_it_delete_value(&iterator);
    }

    return found;
}

void
hash_clear(hash_container_ptr container)
{
    hash_container_el_t *element, *tmp;

    pthread_spin_lock(&container->lock);
    container->last_lock_line = __LINE__;
    HASH_ITER(hh, container->head, element, tmp) {
        HASH_DEL(container->head, element);
        free(element->data);
        element->data = NULL;
        free(element->key);
        free(element);
    }
    pthread_spin_unlock(&container->lock);
}

unsigned int
hash_size(hash_container_ptr container)
{
    if(container->head == 0)
        return 0;
    else
        return container->head->hh.tbl->num_items;
}


hash_iterator_ptr
hash_begin(hash_container_ptr container, hash_iterator_ptr iterator)
{
    if(container == NULL && iterator == NULL) {
        return calloc(1, sizeof(struct hash_iterator));
    }

    if(container->head == NULL)
        return hash_end(container, iterator);

    if(iterator == NULL)
        iterator = malloc(sizeof(struct hash_iterator));

    iterator->current_data = container->head;
    iterator->next = iterator->current_data->hh.next;
    iterator->prev = iterator->current_data->hh.prev;
    iterator->container = container;

    return iterator;
}

hash_iterator_ptr
hash_end(hash_container_ptr container, hash_iterator_ptr iterator)
{
    if(container == NULL && iterator == NULL) {
        return calloc(1, sizeof(struct hash_iterator));
    }

    if(iterator == NULL)
        iterator = malloc(sizeof(struct hash_iterator));

    iterator->current_data = NULL;
    iterator->next = NULL;
    if(container->head == NULL)
        iterator->prev = NULL;
    else
        iterator->prev =
            ELMT_FROM_HH(container->head->hh.tbl,
                         container->head->hh.tbl->tail);
    iterator->container = container;

    return iterator;
}

void
hash_it_destroy(hash_iterator_ptr iterator)
{
    if(iterator)
        free(iterator);
}


hash_iterator_ptr
hash_it_inc(hash_iterator_ptr iterator)
{
    iterator->prev = iterator->current_data;
    iterator->current_data = iterator->next;
    if(iterator->current_data)
        iterator->next = iterator->current_data->hh.next;
    else
        iterator->next = NULL;

    return iterator;
}

hash_iterator_ptr
hash_it_dec(hash_iterator_ptr iterator)
{
    iterator->next = iterator->current_data;
    iterator->current_data = iterator->prev;
    if(iterator->current_data)
        iterator->prev = iterator->current_data->hh.prev;
    else
        iterator->prev = NULL;

    return iterator;
}

bool
hash_it_equ(hash_iterator_ptr iterator, hash_iterator_ptr other_it)
{
    return iterator->current_data == other_it->current_data;
}

hash_iterator_ptr
hash_it_cpy(hash_iterator_ptr iterator, hash_iterator_ptr new_iterator)
{
    if(!new_iterator)
        new_iterator = malloc(sizeof(struct hash_iterator));

    *new_iterator = *iterator;

    return new_iterator;
}

hash_iterator_ptr
hash_it_remove_ref(hash_iterator_ptr iterator)
{
    struct hash_iterator next_it;

    hash_it_cpy(iterator, &next_it);
    hash_it_inc(&next_it);

    pthread_spin_lock(&iterator->container->lock);
    iterator->container->last_lock_line = __LINE__;
    HASH_DEL(iterator->container->head, iterator->current_data);
    pthread_spin_unlock(&iterator->container->lock);

    free(iterator->current_data->key);
    free(iterator->current_data);

    return hash_it_cpy(&next_it, iterator);
}

hash_iterator_ptr
hash_it_delete_value(hash_iterator_ptr iterator)
{
    struct hash_iterator next_it;

    hash_it_cpy(iterator, &next_it);
    hash_it_inc(&next_it);

    pthread_spin_lock(&iterator->container->lock);
    iterator->container->last_lock_line = __LINE__;
    HASH_DEL(iterator->container->head, iterator->current_data);
    pthread_spin_unlock(&iterator->container->lock);

    free(iterator->current_data->data);
    free(iterator->current_data->key);
    free(iterator->current_data);

    return hash_it_cpy(&next_it, iterator);
}


void *
hash_it_value(hash_iterator_ptr iterator)
{
    if(iterator->current_data)
        return iterator->current_data->data;
    else
        return NULL;
}

void *
hash_it_key(hash_iterator_ptr iterator)
{
    if(iterator->current_data)
        return iterator->current_data->hh.key;
    else
        return NULL;
}
