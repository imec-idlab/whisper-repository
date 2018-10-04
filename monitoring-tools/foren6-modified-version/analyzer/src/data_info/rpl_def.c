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
 *         RPL Information
 * \author
 *         Foren6 Team <foren6@cetic.be>
 *         http://cetic.github.io/foren6/credits.html
 */

#include "rpl_def.h"
#include "stddef.h"

void
init_rpl_instance_config(rpl_instance_config_t * config)
{
    config->rpl_instance_id = 0;
    config->has_dodagid = false;
    init_ipv6_addr(&config->dodagid);
    config->has_dio_config = false;
    config->version_number = 0;
    config->mode_of_operation = 0;
}

void
init_rpl_instance_data(rpl_instance_data_t * data)
{
    data->rpl_instance_id = 0;
    data->has_dio_data = false;
    data->grounded = false;
    data->preference = 0;
    data->dtsn = 0;
    data->has_rank = false;
    data->rank = 0;
    data->has_metric = false;
    init_rpl_metric(&data->metric);
}

void
init_rpl_dodag_config(rpl_dodag_config_t * config)
{
    config->auth_enabled = 0;
    config->path_control_size = 0;
    config->dio_interval_min = 0;
    config->dio_interval_max = 0;
    config->dio_redundancy_constant = 0;
    config->max_rank_inc = 0;
    config->min_hop_rank_inc = 0;
    config->default_lifetime = 0;
    config->lifetime_unit = 0;
    config->objective_function = 0;
}

void
init_rpl_prefix(rpl_prefix_t * prefix)
{
    init_prefix(&prefix->prefix);
    prefix->on_link = false;
    prefix->auto_address_config = false;
    prefix->router_address = false;
    prefix->valid_lifetime = 0;
    prefix->preferred_lifetime = 0;
}

void
init_rpl_metric(rpl_metric_t * metric)
{
    metric->type = 0;
    metric->value = 0;
}

void
init_rpl_statistics(rpl_statistics_t * statistics)
{
    statistics->last_dao_timestamp = 0;
    statistics->last_dio_timestamp = 0;
    statistics->max_dao_interval = 0;
    statistics->max_dio_interval = 0;
    statistics->dis = 0;
    statistics->dio = 0;
    statistics->dao = 0;
}

void
init_rpl_errors(rpl_errors_t * errors)
{
    errors->rank_errors = 0;
    errors->forward_errors = 0;
    errors->upward_rank_errors = 0;
    errors->downward_rank_errors = 0;
    errors->route_loop_errors = 0;

    errors->ip_mismatch_errors = 0;
    errors->dodag_version_decrease_errors = 0;
    errors->dodag_mismatch_errors = 0;

    errors->dodag_config_mismatch_errors = 0;
}

void
update_rpl_instance_config_from_dio(rpl_instance_config_t * config,
                                    const rpl_dio_t * dio)
{
    if(!dio)
        return;
    config->rpl_instance_id = dio->rpl_instance_id;
    config->has_dodagid = true;
    config->dodagid = dio->dodagid;
    config->has_dio_config = true;
    config->version_number = dio->version_number;
    config->mode_of_operation = dio->mode_of_operation;
}

void
update_rpl_instance_data_from_dio(rpl_instance_data_t * data,
                                  const rpl_dio_t * dio)
{
    if(!dio)
        return;
    data->has_dio_data = true;
    data->has_rank = true;
    data->rpl_instance_id = dio->rpl_instance_id;
    data->rank = dio->rank;
    data->grounded = dio->grounded;
    data->preference = dio->preference;
    data->dtsn = dio->dtsn;
}

void
update_rpl_instance_data_from_metric(rpl_instance_data_t * data,
                                     const rpl_metric_t * metric)
{
    if(metric) {
        data->has_metric = true;
        data->metric = *metric;
    } else {
        data->has_metric = false;
    }
}

void
update_rpl_instance_data_from_hop_by_hop(rpl_instance_data_t * data,
                                         const rpl_hop_by_hop_opt_t *
                                         hop_by_hop)
{
    if(hop_by_hop) {
        data->has_rank = true;
        data->rank = hop_by_hop->sender_rank;
    }
}

void
update_rpl_instance_config_from_dao(rpl_instance_config_t * config,
                                    const rpl_dao_t * dao)
{
    if(!dao)
        return;
    config->rpl_instance_id = dao->rpl_instance_id;
    if(dao->dodagid_present) {
        config->has_dodagid = true;
        config->dodagid = dao->dodagid;
    }
}

void
update_rpl_instance_data_from_dao(rpl_instance_data_t * data,
                                  const rpl_dao_t * dao)
{
    if(!dao)
        return;
    data->has_dao_data = true;
    data->latest_dao_sequence = dao->dao_sequence;
}

void
rpl_instance_config_delta(const rpl_instance_config_t * left,
                          const rpl_instance_config_t * right,
                          rpl_instance_config_delta_t * delta)
{
    if(delta == NULL)
        return;
    if(left == NULL && right == NULL) {
        delta->rpl_instance_id = false;
        delta->has_dodagid = false;
        delta->dodagid = false;
        delta->has_dio_config = false;
        delta->version_number = false;
        delta->mode_of_operation = false;
    } else if(left == NULL || right == NULL) {
        delta->rpl_instance_id = true;
        delta->has_dodagid = true;
        delta->dodagid = true;
        delta->has_dio_config = true;
        delta->version_number = true;
        delta->mode_of_operation = true;
    } else {
        delta->rpl_instance_id =
            right->rpl_instance_id != left->rpl_instance_id;
        delta->has_dodagid = right->has_dodagid != left->has_dodagid;
        delta->dodagid =
            addr_compare_ip(&right->dodagid, &left->dodagid) != 0;
        delta->has_dio_config = right->has_dio_config != left->has_dio_config;
        delta->version_number = right->version_number != left->version_number;
        delta->mode_of_operation =
            right->mode_of_operation != left->mode_of_operation;
    }
    delta->has_changed = delta->rpl_instance_id || delta->has_dodagid
        || delta->dodagid || delta->has_dio_config || delta->version_number
        || delta->mode_of_operation;
}

void
rpl_instance_data_delta(const rpl_instance_data_t * left,
                        const rpl_instance_data_t * right,
                        rpl_instance_data_delta_t * delta)
{
    if(delta == NULL)
        return;
    if(left == NULL && right == NULL) {
        delta->rpl_instance_id = false;
        delta->has_metric = false;
        delta->metric = 0;
        delta->has_rank = false;
        delta->has_dio_data = false;
        delta->rank = 0;
        delta->grounded = false;
        delta->preference = false;
        delta->dtsn = 0;
        delta->has_dao_data = false;
        delta->latest_dao_sequence = 0;
    } else if(left == NULL || right == NULL) {
        delta->rpl_instance_id = true;
        delta->has_metric = true;
        delta->metric = left ? left->metric.value : right->metric.value;
        delta->has_rank = left ? left->has_rank : right->has_rank;
        delta->rank = left ? left->rank : right->rank;
        delta->has_dio_data = left ? left->has_dio_data : right->has_dio_data;
        delta->grounded = true;
        delta->preference = true;
        delta->dtsn = left ? left->dtsn : right->dtsn;
        delta->has_dao_data = left ? left->has_dao_data : right->has_dao_data;
        delta->latest_dao_sequence =
            left ? left->latest_dao_sequence : right->latest_dao_sequence;
    } else {
        delta->rpl_instance_id =
            right->rpl_instance_id != left->rpl_instance_id;
        delta->has_metric = right->has_metric != left->has_metric;
        delta->metric = right->metric.value - left->metric.value;
        delta->has_rank = left->has_rank != right->has_rank;
        delta->rank = right->rank != left->rank;
        delta->has_dio_data = left->has_dio_data != right->has_dio_data;
        delta->grounded = right->grounded != left->grounded;
        delta->preference = right->preference != left->preference;
        delta->dtsn = right->dtsn != left->dtsn;
        delta->has_dao_data = left->has_dao_data != right->has_dao_data;
        delta->latest_dao_sequence =
            right->latest_dao_sequence - left->latest_dao_sequence;
    }
    delta->has_changed = delta->rpl_instance_id || delta->has_metric
        || delta->metric || delta->has_rank || delta->rank
        || delta->has_dio_data || delta->grounded || delta->preference
        || delta->dtsn || delta->has_dao_data || delta->latest_dao_sequence;
}

void
rpl_dodag_config_delta(const rpl_dodag_config_t * left,
                       const rpl_dodag_config_t * right,
                       rpl_dodag_config_delta_t * delta)
{
    if(delta == NULL)
        return;
    if(left == NULL && right == NULL) {
        delta->auth_enabled = false;
        delta->path_control_size = false;
        delta->dio_interval_min = false;
        delta->dio_interval_max = false;
        delta->dio_redundancy_constant = false;
        delta->max_rank_inc = false;
        delta->min_hop_rank_inc = false;
        delta->default_lifetime = false;
        delta->lifetime_unit = false;
        delta->objective_function = false;
    } else if(left == NULL || right == NULL) {
        delta->auth_enabled = true;
        delta->path_control_size = true;
        delta->dio_interval_min = true;
        delta->dio_interval_max = true;
        delta->dio_redundancy_constant = true;
        delta->max_rank_inc = true;
        delta->min_hop_rank_inc = true;
        delta->default_lifetime = true;
        delta->lifetime_unit = true;
        delta->objective_function = true;
    } else {
        delta->auth_enabled = right->auth_enabled != left->auth_enabled;
        delta->path_control_size =
            right->path_control_size != left->path_control_size;
        delta->dio_interval_min =
            right->dio_interval_min != left->dio_interval_min;
        delta->dio_interval_max =
            right->dio_interval_max != left->dio_interval_max;
        delta->dio_redundancy_constant =
            right->dio_redundancy_constant != left->dio_redundancy_constant;
        delta->max_rank_inc = right->max_rank_inc != left->max_rank_inc;
        delta->min_hop_rank_inc =
            right->min_hop_rank_inc != left->min_hop_rank_inc;
        delta->default_lifetime =
            right->default_lifetime != left->default_lifetime;
        delta->lifetime_unit = right->lifetime_unit != left->lifetime_unit;
        delta->objective_function =
            right->objective_function != left->objective_function;
    }

    delta->has_changed = delta->auth_enabled || delta->path_control_size
        || delta->dio_interval_min || delta->dio_interval_max
        || delta->dio_redundancy_constant || delta->max_rank_inc
        || delta->min_hop_rank_inc || delta->default_lifetime
        || delta->lifetime_unit || delta->objective_function;
}

void
rpl_prefix_delta(const rpl_prefix_t * left, const rpl_prefix_t * right,
                 rpl_prefix_delta_t * delta)
{
    if(delta == NULL)
        return;
    if(left == NULL && right == NULL) {
        delta->prefix = false;
        delta->on_link = false;
        delta->auto_address_config = false;
        delta->router_address = false;
        delta->valid_lifetime = false;
        delta->preferred_lifetime = false;
    } else if(left == NULL || right == NULL) {
        delta->prefix = true;
        delta->on_link = true;
        delta->auto_address_config = true;
        delta->router_address = true;
        delta->valid_lifetime = true;
        delta->preferred_lifetime = true;
    } else {
        delta->prefix = prefix_compare(&right->prefix, &left->prefix) != 0;
        delta->on_link = right->on_link != left->on_link;
        delta->auto_address_config =
            right->auto_address_config != left->auto_address_config;
        delta->router_address = right->router_address != left->router_address;
        delta->valid_lifetime = right->valid_lifetime != left->valid_lifetime;
        delta->preferred_lifetime =
            right->preferred_lifetime != left->preferred_lifetime;
    }

    delta->has_changed = delta->prefix || delta->on_link
        || delta->auto_address_config || delta->router_address
        || delta->valid_lifetime || delta->preferred_lifetime;
}

void
rpl_statistics_delta(const rpl_statistics_t * left,
                     const rpl_statistics_t * right,
                     rpl_statistics_delta_t * delta)
{
    if(delta == NULL)
        return;
    if(left == NULL && right == NULL) {
        delta->max_dao_interval = 0;
        delta->max_dio_interval = 0;
        delta->dis = 0;
        delta->dio = 0;
        delta->dao = 0;
    } else if(left == NULL || right == NULL) {
        delta->max_dao_interval =
            right ? right->max_dao_interval : left->max_dao_interval;
        delta->max_dio_interval =
            right ? right->max_dio_interval : left->max_dio_interval;
        delta->dis = right ? right->dis : left->dis;
        delta->dio = right ? right->dio : left->dio;
        delta->dao = right ? right->dao : left->dao;
    } else {
        delta->max_dao_interval =
            right->max_dao_interval - left->max_dao_interval;
        delta->max_dio_interval =
            right->max_dio_interval - left->max_dio_interval;
        delta->dis = right->dis - left->dis;
        delta->dio = right->dio - left->dio;
        delta->dao = right->dao - left->dao;
    }
    delta->has_changed = delta->max_dao_interval != 0 ||
        delta->max_dio_interval != 0 ||
        delta->dis || delta->dio || delta->dao;
}

void
rpl_errors_delta(const rpl_errors_t * left, const rpl_errors_t * right,
                 rpl_errors_delta_t * delta)
{
    if(delta == NULL)
        return;
    if(left == NULL && right == NULL) {
        delta->rank_errors = 0;
        delta->forward_errors = 0;
        delta->upward_rank_errors = 0;
        delta->downward_rank_errors = 0;
        delta->route_loop_errors = 0;

        delta->ip_mismatch_errors = 0;
        delta->dodag_version_decrease_errors = 0;
        delta->dodag_mismatch_errors = 0;

        delta->dodag_config_mismatch_errors = 0;
    } else if(left == NULL || right == NULL) {
        delta->rank_errors =
            right ? right->rank_errors : left->rank_errors;
        delta->forward_errors =
            right ? right->forward_errors : left->forward_errors;
        delta->upward_rank_errors =
            right ? right->upward_rank_errors : left->upward_rank_errors;
        delta->downward_rank_errors =
            right ? right->downward_rank_errors : left->downward_rank_errors;
        delta->route_loop_errors =
            right ? right->route_loop_errors : left->route_loop_errors;

        delta->ip_mismatch_errors =
            right ? right->ip_mismatch_errors : left->ip_mismatch_errors;
        delta->dodag_version_decrease_errors =
            right ? right->dodag_version_decrease_errors : left->
            dodag_version_decrease_errors;
        delta->dodag_mismatch_errors =
            right ? right->dodag_mismatch_errors : left->
            dodag_mismatch_errors;

        delta->dodag_config_mismatch_errors =
            right ? right->dodag_config_mismatch_errors : left->
            dodag_config_mismatch_errors;
    } else {
        delta->forward_errors =
            right->forward_errors - left->forward_errors;
        delta->rank_errors =
            right->rank_errors - left->rank_errors;
        delta->upward_rank_errors =
            right->upward_rank_errors - left->upward_rank_errors;
        delta->downward_rank_errors =
            right->downward_rank_errors - left->downward_rank_errors;
        delta->route_loop_errors =
            right->route_loop_errors - left->route_loop_errors;

        delta->ip_mismatch_errors =
            right->ip_mismatch_errors - left->ip_mismatch_errors;
        delta->dodag_version_decrease_errors =
            right->dodag_version_decrease_errors -
            left->dodag_version_decrease_errors;
        delta->dodag_mismatch_errors =
            right->dodag_mismatch_errors - left->dodag_mismatch_errors;

        delta->dodag_config_mismatch_errors =
            right->dodag_config_mismatch_errors -
            left->dodag_config_mismatch_errors;
    }

    delta->has_changed =
        delta->rank_errors + delta->forward_errors +
        delta->upward_rank_errors + delta->downward_rank_errors +
        delta->route_loop_errors + delta->ip_mismatch_errors +
        /*delta->dodag_version_decrease_errors + */
        delta->dodag_mismatch_errors +
        delta->dodag_config_mismatch_errors;
}

bool
rpl_instance_config_compare(const rpl_instance_config_t * left,
                            const rpl_instance_config_t * right)
{
    rpl_instance_config_delta_t delta;

    rpl_instance_config_delta(left, right, &delta);
    return delta.has_changed;
}

bool
rpl_dodag_config_compare(const rpl_dodag_config_t * left,
                         const rpl_dodag_config_t * right)
{
    rpl_dodag_config_delta_t delta;

    rpl_dodag_config_delta(left, right, &delta);
    return delta.has_changed;
}

bool
rpl_prefix_compare(const rpl_prefix_t * left, const rpl_prefix_t * right)
{
    rpl_prefix_delta_t delta;

    rpl_prefix_delta(left, right, &delta);
    return delta.has_changed;
}

bool
rpl_instance_data_compare(const rpl_instance_data_t * left,
                          const rpl_instance_data_t * right)
{
    rpl_instance_data_delta_t delta;

    rpl_instance_data_delta(left, right, &delta);
    return delta.has_changed;
}

bool
rpl_statistics_compare(const rpl_statistics_t * left,
                       const rpl_statistics_t * right)
{
    rpl_statistics_delta_t delta;

    rpl_statistics_delta(left, right, &delta);
    return delta.has_changed;
}

bool
rpl_errors_compare(const rpl_errors_t * left, const rpl_errors_t * right)
{
    rpl_errors_delta_t delta;

    rpl_errors_delta(left, right, &delta);
    return delta.has_changed;
}
