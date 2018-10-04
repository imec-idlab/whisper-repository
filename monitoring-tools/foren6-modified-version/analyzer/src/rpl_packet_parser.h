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
 *         RPL Packet Parser
 * \author
 *         Foren6 Team <foren6@cetic.be>
 *         http://cetic.github.io/foren6/credits.html
 */

#ifndef RPL_PACKET_PARSER_H
#define	RPL_PACKET_PARSER_H

#include "data_info/rpl_data.h"
#include "data_collector/rpl_event_callbacks.h"
#include "interface_reader/interfaces_mgr.h"

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct analyzer_callbacks {
    void (*onErrorEvent) (char const *errorMessage);
} analyzer_callbacks_t;

typedef struct analyser_config {
    bool old_tshark;
    int root_rank;
    struct in6_addr context0;
    bool address_autconf_only;
    bool one_preferred_parent;
    bool sender_rank_inverted;
} analyser_config_t;

    /**
 * Initialize the analyzer.
 */
void rpl_tool_init();

void rpl_tool_start();

void rpl_tool_cleanup();

void rpl_tool_start_capture();

void rpl_tool_stop_capture();

    /**
 * Configure the analysis parameter
 */
void rpl_tool_set_analyser_config(const analyser_config_t * config);
const analyser_config_t *rpl_tool_get_analyser_config();

interface_t *rpl_tool_get_interface(const char *name);

    /**
 * Set callback to call when events are triggered (like node creation)
 * @param callbacks
 */
void rpl_tool_set_callbacks(rpl_event_callbacks_t * callbacks);

/**
 * Set callback to call when analyzer events are triggered (like error reporting)
 * @param callbacks
 */
void rpl_tool_set_analyzer_callbacks(analyzer_callbacks_t * callbacks);

void rpl_tool_report_error(char const *error_message);

#ifdef	__cplusplus
}
#endif
#endif                          /* RPL_PACKET_PARSER_H */
