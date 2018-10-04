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
 *         RPL Metrics
 * \author
 *         Foren6 Team <foren6@cetic.be>
 *         http://cetic.github.io/foren6/credits.html
 */

#ifndef METRIC_H
#define	METRIC_H

#include <stdint.h>

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct di_metric_type {
    const char *name;
    char *(*to_string) (uint64_t value);
    double (*to_display_value) (uint64_t value);
} di_metric_type_t;

typedef struct di_metric_type_el {
    di_metric_type_t *type;
    struct di_metric_type_el *next;
} di_metric_type_el_t, *di_metric_type_list_t;

typedef struct di_metric {
    di_metric_type_t *type;
    uint64_t value;
} di_metric_t;

typedef void (*metric_enum_callback_t) (di_metric_type_t * metric_type);

void metric_add_type(const di_metric_type_t * metric_model);
di_metric_type_t *metric_get_type(const char *name);
char *metric_to_string(const di_metric_t * metric_value);
double metric_get_display_value(const di_metric_t * metric_value);
void metric_enumerate(metric_enum_callback_t callback);

#ifdef	__cplusplus
}
#endif
#endif                          /* METRIC_H */
