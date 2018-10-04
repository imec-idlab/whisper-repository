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
 *         Handle polling on file descriptor
 * \author
 *         Foren6 Team <foren6@cetic.be>
 *         http://cetic.github.io/foren6/credits.html
 */

#include "descriptor_poll.h"
#include <sys/select.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#define EPOLL_MAX_EVENTS 5
#define SELECT_FD_NUM 10

typedef struct desc_poll_info {
    int fd;
    ready_callback callback;
    void *user_data;
} desc_poll_info_t;

static pthread_t poll_thread;

static bool stop_thread;

static desc_poll_info_t *poll_data[SELECT_FD_NUM] = { 0 };
static int poll_number = 0;
static pthread_mutex_t poll_mutex;
static int last_lock_line;
static int poll_abort_pipe[2];

static void *desc_poll_run(void *data);

void
desc_poll_init()
{
    static bool initialized = false;

    if(initialized)
        return;

    initialized = true;

    pipe(poll_abort_pipe);
    fcntl(poll_abort_pipe[1], F_SETFL, O_NONBLOCK);
    fcntl(poll_abort_pipe[0], F_SETFL, O_NONBLOCK);

    memset(poll_data, 0, sizeof(poll_data));
    poll_number = 0;

    pthread_mutex_init(&poll_mutex, NULL);
    last_lock_line = __LINE__;
    stop_thread = false;

    pthread_create(&poll_thread, NULL, &desc_poll_run, NULL);
}

void
desc_poll_cleanup()
{
    int i;

    stop_thread = true;
    write(poll_abort_pipe[1], "c", 1);  //write a byte (the string avoids declaring a dummy variable) to abort the select as we want to stop the thread
    pthread_join(poll_thread, NULL);
    pthread_mutex_destroy(&poll_mutex);

    for(i = 0; i < SELECT_FD_NUM; i++) {
        if(poll_data[i])
            free(poll_data[i]);
    }

    close(poll_abort_pipe[1]);
    close(poll_abort_pipe[0]);
}

bool
desc_poll_add(int fd, ready_callback callback, void *user_data)
{
    int i;
    sigset_t x;
    desc_poll_info_t *data =
        (desc_poll_info_t *) malloc(sizeof(desc_poll_info_t));

    data->fd = fd;
    data->callback = callback;
    data->user_data = user_data;

    sigemptyset(&x);
    sigaddset(&x, SIGPIPE);

    sigprocmask(SIG_BLOCK, &x, NULL);
    pthread_mutex_lock(&poll_mutex);
    last_lock_line = __LINE__;
    for(i = 0; i < SELECT_FD_NUM; i++) {
        if(poll_data[i] == NULL) {
            poll_data[i] = data;
            poll_number++;
            write(poll_abort_pipe[1], "a", 1);  //write a byte (the string avoids declaring a dummy variable) to abort the select as we added a new file descriptor
            break;
        } else if(poll_data[i]->fd == fd) {
            free(data);
            data = poll_data[i];
            data->callback = callback;
            data->user_data = user_data;
            break;
        }
    }
    pthread_mutex_unlock(&poll_mutex);
    sigprocmask(SIG_UNBLOCK, &x, NULL);

    if(i < SELECT_FD_NUM)
        return true;

    free(data);

    return false;
}

void
desc_poll_del(int fd)
{
    int i;
    sigset_t x;

    sigemptyset(&x);
    sigaddset(&x, SIGPIPE);

    sigprocmask(SIG_BLOCK, &x, NULL);
    pthread_mutex_lock(&poll_mutex);
    last_lock_line = __LINE__;
    for(i = 0; i < SELECT_FD_NUM; i++) {
        if(poll_data[i] && poll_data[i]->fd == fd) {
            break;
        }
    }

    if(poll_data[i] && i < SELECT_FD_NUM) {
        write(poll_abort_pipe[1], "d", 1);      //write a byte (the string avoids declaring a dummy variable) to abort the select as we remove a file descriptor
        free(poll_data[i]);
        poll_data[i] = NULL;
        poll_number--;
    }
    pthread_mutex_unlock(&poll_mutex);
    sigprocmask(SIG_UNBLOCK, &x, NULL);
}

static void *
desc_poll_run(void *data)
{
    while(stop_thread == false)
        desc_poll_process_events();

    return NULL;
}

void
desc_poll_process_events()
{
    int maxfd, i, retval;
    fd_set read_set;
    sigset_t x;

    sigemptyset(&x);
    sigaddset(&x, SIGPIPE);

    FD_ZERO(&read_set);

    sigprocmask(SIG_BLOCK, &x, NULL);
    pthread_mutex_lock(&poll_mutex);
    last_lock_line = __LINE__;
    for(maxfd = -1, i = 0; i < SELECT_FD_NUM; i++) {
        if(poll_data[i]) {
            FD_SET(poll_data[i]->fd, &read_set);
            if(maxfd < poll_data[i]->fd)
                maxfd = poll_data[i]->fd;
        }
    }
    pthread_mutex_unlock(&poll_mutex);
    sigprocmask(SIG_UNBLOCK, &x, NULL);

    //No descriptor to poll, just wait and return (to avoid wasting cpu time)
    if(maxfd < 0) {
        usleep(100000);
        return;
    }
    //Add a pipe to be able to abort the select call when adding new file descriptor to poll
    if(poll_abort_pipe[0] > maxfd)
        maxfd = poll_abort_pipe[0];
    FD_SET(poll_abort_pipe[0], &read_set);

    retval = select(maxfd + 1, &read_set, NULL, NULL, NULL);
    if(retval == -1)
        perror("select failed");
    else if(retval > 0) {
        sigprocmask(SIG_BLOCK, &x, NULL);
        pthread_mutex_lock(&poll_mutex);
        last_lock_line = __LINE__;
        for(i = 0; i < SELECT_FD_NUM; i++) {
            if(poll_data[i] && FD_ISSET(poll_data[i]->fd, &read_set)) {
                desc_poll_info_t *current_info = poll_data[i];
                int fd = poll_data[i]->fd;
                void *user_data = poll_data[i]->user_data;

                pthread_mutex_unlock(&poll_mutex);
                current_info->callback(fd, user_data);
                pthread_mutex_lock(&poll_mutex);
                last_lock_line = __LINE__;
            }
        }
        pthread_mutex_unlock(&poll_mutex);
        sigprocmask(SIG_UNBLOCK, &x, NULL);

        if(FD_ISSET(poll_abort_pipe[0], &read_set)) {
            char dummy;

            read(poll_abort_pipe[0], &dummy, 1);
        }

    }
}
