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

#include "rpl_packet_parser.h"
#include "descriptor_poll.h"
#include "interface_reader/interfaces_mgr.h"
#include "sniffer_packet_parser.h"
#include <stdio.h>

static analyzer_callbacks_t analyzer_callbacks = { 0 };
static analyser_config_t analyser_config;

void
rpl_tool_set_callbacks(rpl_event_callbacks_t * callbacks)
{
    rpl_event_set_callbacks(callbacks);
}

void
rpl_tool_init()
{
    desc_poll_init();
    rpldata_init();
    analyser_config.old_tshark = false;
    analyser_config.root_rank = 256;
    analyser_config.context0.s6_addr16[0] = 0xaaaa;
    analyser_config.context0.s6_addr16[1] = 0;
    analyser_config.context0.s6_addr16[2] = 0;
    analyser_config.context0.s6_addr16[3] = 0;
    analyser_config.context0.s6_addr16[4] = 0;
    analyser_config.context0.s6_addr16[5] = 0;
    analyser_config.context0.s6_addr16[6] = 0;
    analyser_config.context0.s6_addr16[7] = 0;
    analyser_config.address_autconf_only = true;
    analyser_config.one_preferred_parent = true;
    analyser_config.sender_rank_inverted = false;
}

void
rpl_tool_start()
{
    sniffer_parser_init();
}

void
rpl_tool_cleanup()
{
    desc_poll_cleanup();
    sniffer_parser_cleanup();
}

void
rpl_tool_start_capture()
{
    sniffer_parser_reset();
    sniffer_parser_create_out();
}

void
rpl_tool_stop_capture()
{
    sniffer_parser_close_out();
}

void
rpl_tool_set_analyser_config(const analyser_config_t * config)
{
    if(!config)
        return;
    analyser_config = *config;
}

const analyser_config_t *
rpl_tool_get_analyser_config()
{
    return &analyser_config;
}

interface_t *
rpl_tool_get_interface(const char *name)
{
    return interfacemgr_get(name);
}

void
rpl_tool_set_analyzer_callbacks(analyzer_callbacks_t * callbacks)
{
    analyzer_callbacks = *callbacks;
}

void
rpl_tool_report_error(char const *error_message)
{
    fprintf(stderr, "%s\n", error_message);
    if(analyzer_callbacks.onErrorEvent)
        analyzer_callbacks.onErrorEvent(error_message);
}
