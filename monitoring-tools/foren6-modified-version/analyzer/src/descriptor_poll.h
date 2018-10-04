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

#ifndef DESCRIPTOR_POLL_H
#define	DESCRIPTOR_POLL_H

#include <stdbool.h>

#ifdef	__cplusplus
extern "C" {
#endif

typedef void (*ready_callback) (int, void *);

void desc_poll_init();
void desc_poll_cleanup();

bool desc_poll_add(int fd, ready_callback callback, void *user_data);
void desc_poll_del(int fd);

void desc_poll_process_events();

#ifdef	__cplusplus
}
#endif
#endif                          /* DESCRIPTOR_POLL_H */
