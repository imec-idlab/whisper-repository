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
 *         Parser Registration
 * \author
 *         Foren6 Team <foren6@cetic.be>
 *         http://cetic.github.io/foren6/credits.html
 */

#include "parser_register.h"

#include <stdlib.h>
#include <stdbool.h>
#include "../utlist.h"

#include "rpl_parser.h"
#include "sixtop_parser.h"

typedef struct parser_el {
    parser_t parser;
    struct parser_el *next;
} parser_el_t, *parser_list_t;

parser_list_t parsers;

static void parser_add(parser_t parser);

void
parser_register_all()
{
    parsers = NULL;

    parser_add(rpl_parser_register());
    parser_add(sixtop_parser_register());
}

void
parser_cleanup()
{
	parser_el_t *parser_element;

    LL_FOREACH(parsers, parser_element)
        parser_element->parser.cleanup();
}

void
parser_begin_packet()
{
    parser_el_t *parser_element;

    LL_FOREACH(parsers, parser_element)
        parser_element->parser.begin_packet();
}

void
parser_parse_field(const char *nameStr, const char *showStr,
                   const char *valueStr, int64_t valueInt)
{
    parser_el_t *parser_element;

    LL_FOREACH(parsers, parser_element)
        parser_element->parser.parse_field(nameStr, showStr, valueStr,
                                           valueInt);
}

void
parser_end_packet()
{
    parser_el_t *parser_element;

    LL_FOREACH(parsers, parser_element)
        parser_element->parser.end_packet();
}

static void
parser_add(parser_t parser)
{
    parser_el_t *parser_element;

    parser_element = (parser_el_t *) malloc(sizeof(parser_el_t));
    parser_element->parser = parser;
    LL_PREPEND(parsers, parser_element);
}
