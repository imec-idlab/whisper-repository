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
 *         6TOP Messages Parser
 * \author
 *         Christophe Verdonck <Christophe.Verdonck@student.uantwerpen.be>
 */

#ifndef SIXTOP_PARSER_H
#define	SIXTOP_PARSER_H

#include "parser_register.h"

#ifdef	__cplusplus
extern "C" {
#endif
	
parser_t sixtop_parser_register();

#ifdef	__cplusplus
}
#endif
#endif                          /* SIXTOP_PARSER_H */
