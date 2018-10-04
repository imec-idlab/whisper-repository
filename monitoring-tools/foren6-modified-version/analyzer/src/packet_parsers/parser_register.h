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

#ifndef PARSER_REGISTER_H
#define	PARSER_REGISTER_H

#include <stdint.h>

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct parser {
    const char *parser_name;
	void (*cleanup) ();
    void (*begin_packet) ();
    void (*parse_field) (const char *nameStr, const char *showStr,
                         const char *valueStr, int64_t valueInt);
    void (*end_packet) ();
} parser_t;

void parser_register_all();
void parser_cleanup();

void parser_begin_packet();
void parser_parse_field(const char *nameStr, const char *showStr,
                        const char *valueStr, int64_t valueInt);
void parser_end_packet();

#ifdef	__cplusplus
}
#endif
#endif                          /* PARSER_REGISTER_H */
