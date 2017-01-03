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
 *
 *
 * This code is hacky, rushed and incomplete.  It does not support all, or many,
 * of the OSC tag types.  It supports a basic message (with a simple address
 * format) with integer and/or float types.
 */

#include <SmingCore/SmingCore.h>
#include <Wiring/Wstring.h>
#include "osc.h"

#define MAX_CBS 	16
#define MAX_PFX_LEN	32

#define MAX_ARGS 	16

typedef struct {
	String 	prefix;
	cb_t 	callback;
} osc_handler_t;

static osc_handler_t osc_handlers[MAX_CBS] = {0};

typedef struct {
	uint32_t 	size;
	unsigned char 	contents;
} osc_element_t;

typedef struct {
	char		bundle[8];
	uint64_t	timetag;
} osc_bundle_t;

/*****************************************************************************/

/* Register a callback for a given address string. */
void	osc_reg_handler(const char *prefix, cb_t callback)
{
	for (int i = 0; i < MAX_CBS; i++) {
		if (osc_handlers[i].prefix.equals("")) {
			osc_handlers[i].prefix.setString(prefix);
			osc_handlers[i].callback = callback;
			return;
		}
	}
	debugf("No room for handler registration.\n");
}

static cb_t osc_match_cb(const char *addr)
{
	// Match the first part of addr, if prefix is shorter
	for (int i = 0; i < MAX_CBS; i++) {
		String s(addr);
		if (s.startsWith(osc_handlers[i].prefix)) {
			return osc_handlers[i].callback;
		}
	}
	return 0;
}

#define ROUND_NEXT_WORD(x) ((__typeof__(x)) (((uintptr_t)(x) + 3) & ~3) )

#define BE32(x) ( (((x) & 0xff) << 24) | (((x) & 0xff00) << 8) | (((x) & 0xff0000) >> 8) | (((x) & 0xff000000) >> 24) )

static void osc_parse_element(const uint8_t *e)
{
	/* This string format is "(/addresspattern)(,typetag)+"
	 * the address pattern length is padded w/ zero up to 4 byte boundary.
	 * The type tags are padded to a 4 byte boundary.
	 * The argument data is naturally-aligned (Note, doubles too!), padded to align.
	 */
	const char *addr = (const char *)e;
	const uint8_t *i = e;
	// Find end:
	while (*i++) {}
	// i points to byte after address.
#ifdef DEBUG
	debugf("Element address %s\n", addr);
#endif

	// Round i up to nearest 4 bytes.  If it's already aligned, it means the
	// 0 is in a new word:
	i = ROUND_NEXT_WORD(i);

	const char *tags = (const char *)i;
	if (*tags != ',') {
		debugf("Tags string not found (starts with 0x%x)\r\n", *tags);
		return;
	}
	tags++;

	const uint8_t *tag_data = i;
	// Skip over tags string:
	while (*tag_data++) {}
	tag_data = ROUND_NEXT_WORD(tag_data);

#ifdef DEBUG
	debugf("Tag string '%s', starts at +%d, data at +%d\r\n",
	       tags, (const uint8_t *)tags - e, tag_data - e);
#endif

	int argc = 0;
	osc_arg argv[MAX_ARGS];

	while (*tags && argc < MAX_ARGS) {
		char t = *tags++;

		switch (t) {
		case 'i':
			argv[argc].u.i = BE32(*(uint32_t *)tag_data);
			argv[argc].type = INT32;
			tag_data += 4;
			break;
		case 'f':
			// Puns are funny!
			argv[argc].u.i = BE32(*(uint32_t *)tag_data);
			argv[argc].type = FLOAT;
			tag_data += 4;
			break;
		default:
			debugf("OSC tag type '%c' unknown\r\n", t);
			return;
		}
		argc++;
	}

	cb_t fn = osc_match_cb(addr);
	if (fn) {
		fn(addr, argc, argv);
	} else {
		debugf("Didn't find function for address '%s'", addr);
	}
	// Todo:  return tags (position after parsing finished)
}


/* Given a packet payload, parse and call appropriate callbacks. */
void	osc_parse_msg(const char *msg)
{
	osc_bundle_t *om = (osc_bundle_t *)msg;

	if (strncmp(om->bundle, "#bundle", 8) == 0) {
		const uint8_t 	*first_element = (const uint8_t *)msg + 16;
		int len = BE32(*(uint32_t*)first_element);
#ifdef DEBUG
		debugf("Got element, length %d\n", len);
#endif
		osc_parse_element(first_element + 4);
	} else {
		debugf("Msg doesn't start #bundle.\n");
	}
}
