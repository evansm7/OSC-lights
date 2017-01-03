/* Copyright (c) 2015  Matt Evans
 * A trivial OSC message parser.  Created 17th June 2015.
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
 */

#ifndef OSC_H
#define OSC_H

typedef enum { INT32, FLOAT } arg_t;
struct osc_arg {
	arg_t type;
	union {
		uint32_t 	i;
		float		f;
	} u;
};

typedef osc_arg * osc_args_t;

typedef void (*cb_t)(const char *prefix, int argc, osc_args_t argv);

void	osc_reg_handler(const char *prefix, cb_t callback);
void	osc_parse_msg(const char *msg);

#endif
