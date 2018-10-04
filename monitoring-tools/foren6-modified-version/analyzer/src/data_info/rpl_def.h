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

#ifndef RPL_DEF_H
#define RPL_DEF_H

#include <stdbool.h>
#include <stdint.h>
#include <netinet/in.h>
#include <sys/time.h>
#include "address.h"

#ifdef  __cplusplus
extern "C" {
#endif

typedef enum tag_di_rpl_mop_e {
    RDMOP_NoDownwardRoute,
    RDMOP_NonStoring,
    RDMOP_StoringWithoutMulticast,
    RDMOP_StoringWithMulticast
} di_rpl_mop_e;

typedef enum rpl_dio_opt_metricype_e {
    RDOMT_ETX = 7
} rpl_dio_opt_metric_type_e;

typedef struct rpl_dio_opt_metric {
    rpl_dio_opt_metric_type_e type;
    uint32_t value;
} rpl_dio_opt_metric_t;

typedef struct rpl_dio_opt_prefix {
    di_prefix_t prefix;
    bool on_link;
    bool auto_address_config;
    bool router_address;
    uint32_t valid_lifetime;
    uint32_t preferred_lifetime;
} rpl_dio_opt_prefix_t;

typedef struct rpl_dio_opt_route {
    uint8_t preference;
    uint32_t lifetime;
    uint8_t prefix_bit_length;
    struct in6_addr route_prefix;
} rpl_dio_opt_route_t;

typedef enum tag_di_objective_function_e {
    ROF_ETX = 1
} di_objective_function_e;

typedef struct rpl_dio_opt_config {
    uint16_t auth_enabled;
    uint8_t path_control_size;
    uint8_t dio_interval_min;
    uint8_t dio_interval_max;
    uint8_t dio_redundancy_constant;
    uint16_t max_rank_inc;
    uint16_t min_hop_rank_inc;
    uint8_t default_lifetime;
    uint16_t lifetime_unit;
    di_objective_function_e objective_function;
} rpl_dio_opt_config_t;


//DIO
typedef struct rpl_dio {
    uint8_t rpl_instance_id;
    uint8_t version_number;
    uint16_t rank;
    bool grounded;
    uint8_t preference;
    di_rpl_mop_e mode_of_operation;
    uint8_t dtsn;
    struct in6_addr dodagid;
} rpl_dio_t;

//DAO
typedef struct rpl_dao {
    uint8_t rpl_instance_id;
    bool want_ack;
    bool dodagid_present;
    struct in6_addr dodagid;
    uint8_t dao_sequence;
} rpl_dao_t;

typedef struct rpl_dao_opt_target {
    uint8_t target_bit_length;
    struct in6_addr target;
} rpl_dao_opt_target_t;

typedef struct rpl_dao_opt_transit {
    bool external;
    bool parent_addr_present;
    uint8_t path_control;
    uint8_t path_sequence;
    uint8_t path_lifetime;
    struct in6_addr parent_address;
    //Incomplete
} rpl_dao_opt_transit_t;

//DAO-ACK
//Rare ?
typedef enum tag_rpl_dao_ack_status_e {
    DAO_ACK_STATUS_Ok,
    DAO_ACK_STATUS_ShouldNotBeParent,
    DAO_ACK_STATUS_CantBeParent
} rpl_dao_ack_status_e;

typedef struct rpl_dao_ack {
    uint8_t rpl_instance_id;
    bool dodagid_present;
    struct in6_addr dodagid;
    uint8_t dao_sequence;
    rpl_dao_ack_status_e status;
} rpl_dao_ack_t;

typedef struct rpl_dis_opt_info_req {
    bool instance_predicate;
    uint8_t rpl_instance_id;
    bool version_predicate;
    uint8_t rpl_version;
    bool dodag_predicate;
    struct in6_addr dodagid;
} rpl_dis_opt_info_req_t;

//In data packets
typedef struct rpl_hop_by_hop_opt {
    bool packet_toward_root;
    bool rank_error;
    bool forwarding_error;
    uint8_t rpl_instance_id;
    uint16_t sender_rank;
} rpl_hop_by_hop_opt_t;

///////////////////////////////////////////////////////////////////////////////
// Foren6 data

typedef struct rpl_instance_config {
    uint8_t rpl_instance_id;
    bool has_dodagid;
    struct in6_addr dodagid;
    bool has_dio_config;
    uint8_t version_number;
    di_rpl_mop_e mode_of_operation;
} rpl_instance_config_t;

//Metric configuration is the same as DIO metric option
typedef rpl_dio_opt_metric_t rpl_metric_t;

//Route information is the same as DIO route option
typedef rpl_dio_opt_route_t rpl_route_t;

//DODAG config is the same as DIO config option
typedef rpl_dio_opt_config_t rpl_dodag_config_t;

//Prefix information is the same as DIO prefix  option
typedef rpl_dio_opt_prefix_t rpl_prefix_t;

typedef struct rpl_instance_data {
    uint8_t rpl_instance_id;

    bool has_rank;
    uint16_t rank;

    bool has_dio_data;
    bool grounded;
    uint8_t preference;
    uint8_t dtsn;

    bool has_dao_data;
    uint8_t latest_dao_sequence;

    bool has_metric;
    rpl_metric_t metric;
} rpl_instance_data_t;

// DELTA

typedef struct rpl_instance_config_delta {
    bool has_changed;
    bool rpl_instance_id;
    bool has_dodagid;
    bool dodagid;
    bool has_dio_config;
    bool version_number;
    bool mode_of_operation;
} rpl_instance_config_delta_t;

typedef struct rpl_instance_data_delta {
    bool has_changed;
    bool rpl_instance_id;
    bool has_metric;
    int metric;
    bool has_rank;
    int rank;
    bool has_dio_data;
    bool grounded;
    bool preference;
    int dtsn;
    bool has_dao_data;
    uint8_t latest_dao_sequence;
} rpl_instance_data_delta_t;

typedef struct rpl_dodag_config_delta {
    bool has_changed;
    bool auth_enabled;
    bool path_control_size;
    bool dio_interval_min;
    bool dio_interval_max;
    bool dio_redundancy_constant;
    bool max_rank_inc;
    bool min_hop_rank_inc;
    bool default_lifetime;
    bool lifetime_unit;
    bool objective_function;
} rpl_dodag_config_delta_t;

typedef struct rpl_prefix_delta {
    bool has_changed;
    bool prefix;
    bool on_link;
    bool auto_address_config;
    bool router_address;
    bool valid_lifetime;
    bool preferred_lifetime;
} rpl_prefix_delta_t;

typedef struct rpl_statistics {
    double last_dao_timestamp;
    double last_dio_timestamp;
    double max_dao_interval;        //maximum interval seen between 2 DAO packets
    double max_dio_interval;        //maximum interval seen between 2 DIO packets
    int dis;
    int dio;
    int dao;
    int dao_ack;
} rpl_statistics_t;

typedef struct rpl_statistics_delta {
    bool has_changed;
    double last_dao_timestamp;
    double last_dio_timestamp;
    double max_dao_interval;
    double max_dio_interval;
    int dis;
    int dio;
    int dao;
    int dao_ack;
} rpl_statistics_delta_t;

typedef struct rpl_errors {
    int rank_errors; //incremented when rank error flag is detected
    int forward_errors; //incremented when forward error flag is detected
    int upward_rank_errors; //incremented when data trafic goes from a node to it's parent in the case of the parent has a greater rank than the child
    int downward_rank_errors;       //incremented when data trafic goes from a node to a child where the child have a smaller rank than the parent
    int route_loop_errors;  //incremented when a node choose a parent that is in it's routing table

    int ip_mismatch_errors; //incremented when a DAO message is sent to a node with the wrong IP<=>WPAN addresses association
    int dodag_version_decrease_errors;      //incremented when a DIO message contain a dodag version smaller than the known version
    int dodag_mismatch_errors;      //incremented when a DAO is sent to a parent with the dodagid in the DAO packet different from the parent's dodag
    int dodag_config_mismatch_errors;       //incremented when the dodag configuration advertised by the node is not the same as the dodag root
} rpl_errors_t;

typedef struct rpl_errors_delta {
    int has_changed;

    int rank_errors;
    int forward_errors;
    int upward_rank_errors;
    int downward_rank_errors;
    int route_loop_errors;

    int ip_mismatch_errors;
    int dodag_version_decrease_errors;
    int dodag_mismatch_errors;

    int dodag_config_mismatch_errors;
} rpl_errors_delta_t;

void init_rpl_instance_config(rpl_instance_config_t * config);
void init_rpl_instance_data(rpl_instance_data_t * data);
void init_rpl_dodag_config(rpl_dodag_config_t * config);
void init_rpl_prefix(rpl_prefix_t * prefix);
void init_rpl_metric(rpl_metric_t * metric);
void init_rpl_statistics(rpl_statistics_t * statistics);
void init_rpl_errors(rpl_errors_t * errors);

void update_rpl_instance_config_from_dio(rpl_instance_config_t * config,
                                         const rpl_dio_t * dio);
void update_rpl_instance_data_from_dio(rpl_instance_data_t * data,
                                       const rpl_dio_t * dio);
void update_rpl_instance_data_from_metric(rpl_instance_data_t * data,
                                          const rpl_metric_t * metric);
void update_rpl_instance_data_from_hop_by_hop(rpl_instance_data_t * data,
                                              const rpl_hop_by_hop_opt_t *
                                              hop_by_hop);
void update_rpl_instance_config_from_dao(rpl_instance_config_t * config,
                                         const rpl_dao_t * dao);
void update_rpl_instance_data_from_dao(rpl_instance_data_t * data,
                                       const rpl_dao_t * dao);

void rpl_instance_config_delta(const rpl_instance_config_t * left,
                               const rpl_instance_config_t * right,
                               rpl_instance_config_delta_t * delta);
void rpl_dodag_config_delta(const rpl_dodag_config_t * left,
                            const rpl_dodag_config_t * right,
                            rpl_dodag_config_delta_t * delta);
void rpl_prefix_delta(const rpl_prefix_t * left,
                      const rpl_prefix_t * right,
                      rpl_prefix_delta_t * delta);
void rpl_instance_data_delta(const rpl_instance_data_t * left,
                             const rpl_instance_data_t * right,
                             rpl_instance_data_delta_t * delta);
void rpl_statistics_delta(const rpl_statistics_t * left,
                          const rpl_statistics_t * right,
                          rpl_statistics_delta_t * delta);
void rpl_errors_delta(const rpl_errors_t * left,
                      const rpl_errors_t * right,
                      rpl_errors_delta_t * delta);

bool rpl_instance_config_compare(const rpl_instance_config_t * left,
                                 const rpl_instance_config_t * right);
bool rpl_dodag_config_compare(const rpl_dodag_config_t * left,
                              const rpl_dodag_config_t * right);
bool rpl_prefix_compare(const rpl_prefix_t * left,
                        const rpl_prefix_t * right);
bool rpl_instance_data_compare(const rpl_instance_data_t * left,
                               const rpl_instance_data_t * right);
bool rpl_statistics_compare(const rpl_statistics_t * left,
                            const rpl_statistics_t * right);
bool rpl_errors_compare(const rpl_errors_t * left,
                        const rpl_errors_t * right);

#ifdef  __cplusplus
}
#endif
#endif                          /* RPL_DEF_H */
