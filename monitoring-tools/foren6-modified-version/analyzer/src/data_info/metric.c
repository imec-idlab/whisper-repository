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

#include "metric.h"
#include "../utlist.h"
#include <stdlib.h>
#include <string.h>

static di_metric_type_list_t metrics = NULL;

void
metric_add_type(const di_metric_type_t * metric_model)
{
    di_metric_type_el_t *metric_type;

    metric_type = calloc(1, sizeof(di_metric_type_el_t));
    metric_type->type = calloc(1, sizeof(di_metric_type_t));
    memcpy(metric_type->type, metric_model, sizeof(di_metric_type_t));
    LL_PREPEND(metrics, metric_type);
}

di_metric_type_t *
metric_get_type(const char *name)
{
    di_metric_type_el_t *metric_type;

    LL_FOREACH(metrics, metric_type) {
        if(!strcmp(metric_type->type->name, name))
            return metric_type->type;
    }

    return NULL;
}

char *
metric_to_string(const di_metric_t * metric_value)
{
    if(metric_value->type && metric_value->type->to_string)
        return metric_value->type->to_string(metric_value->value);
    else
        return NULL;
}


double
metric_get_display_value(const di_metric_t * metric_value)
{
    if(metric_value->type && metric_value->type->to_display_value)
        return metric_value->type->to_display_value(metric_value->value);
    else
        return metric_value->value;
}

void
metric_enumerate(metric_enum_callback_t callback)
{
    di_metric_type_el_t *metric_type;

    LL_FOREACH(metrics, metric_type) {
        callback(metric_type->type);
    }
}
