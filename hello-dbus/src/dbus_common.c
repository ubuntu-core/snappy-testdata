/* dbus_common.c
 *
 * Copyright (C) 2003 Philip Blundell <philb@gnu.org>
 * Copyright (C) 2013 Canonical, Ltd.
 *
 * Originally dbus-send.c from the dbus package. It has been heavily modified
 * to work within the regression test framework.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "dbus_common.h"

const char *type_to_name(int message_type)
{
	switch (message_type) {
	case DBUS_MESSAGE_TYPE_SIGNAL:
		return "signal";
	case DBUS_MESSAGE_TYPE_METHOD_CALL:
		return "method call";
	case DBUS_MESSAGE_TYPE_METHOD_RETURN:
		return "method return";
	case DBUS_MESSAGE_TYPE_ERROR:
		return "error";
	default:
		return "(unknown message type)";
	}
}

void log_message(int log_fd, const char *prefix, DBusMessage * message)
{
	const char *sender = NULL;
	const char *destination = NULL;
	const char *unique = "(UNIQUE)";
	int message_type;

	if (log_fd < 0)
		return;

	message_type = dbus_message_get_type(message);
	sender = dbus_message_get_sender(message);
	destination = dbus_message_get_destination(message);

	/* Remove unique (random) names from the logs since they make it
	 * impossible to do simple log comparisons between two different test
	 * runs.
	 */
	if (sender && sender[0] == ':')
		sender = unique;
	if (destination && destination[0] == ':')
		destination = unique;

	dprintf(log_fd, "%s%s sender=%s -> dest=%s",
		prefix, type_to_name(message_type),
		sender ? sender : "(null)",
		destination ? destination : "(null)");

	switch (message_type) {
	case DBUS_MESSAGE_TYPE_METHOD_CALL:
	case DBUS_MESSAGE_TYPE_SIGNAL:
		dprintf(log_fd, " path=%s; interface=%s; member=%s\n",
			dbus_message_get_path(message),
			dbus_message_get_interface(message),
			dbus_message_get_member(message));
		break;

	case DBUS_MESSAGE_TYPE_ERROR:
		dprintf(log_fd, " error_name=%s\n",
			dbus_message_get_error_name(message));
		break;

	default:
		dprintf(log_fd, "\n");
		break;
	}
}

void append_arg(DBusMessageIter * iter, int type, const char *value)
{
	dbus_uint16_t uint16;
	dbus_int16_t int16;
	dbus_uint32_t uint32;
	dbus_int32_t int32;
	dbus_uint64_t uint64;
	dbus_int64_t int64;
	double d;
	unsigned char byte;
	dbus_bool_t v_BOOLEAN;

	/* FIXME - we are ignoring OOM returns on all these functions */
	switch (type) {
	case DBUS_TYPE_BYTE:
		byte = strtoul(value, NULL, 0);
		dbus_message_iter_append_basic(iter, DBUS_TYPE_BYTE, &byte);
		break;

	case DBUS_TYPE_DOUBLE:
		d = strtod(value, NULL);
		dbus_message_iter_append_basic(iter, DBUS_TYPE_DOUBLE, &d);
		break;

	case DBUS_TYPE_INT16:
		int16 = strtol(value, NULL, 0);
		dbus_message_iter_append_basic(iter, DBUS_TYPE_INT16, &int16);
		break;

	case DBUS_TYPE_UINT16:
		uint16 = strtoul(value, NULL, 0);
		dbus_message_iter_append_basic(iter, DBUS_TYPE_UINT16, &uint16);
		break;

	case DBUS_TYPE_INT32:
		int32 = strtol(value, NULL, 0);
		dbus_message_iter_append_basic(iter, DBUS_TYPE_INT32, &int32);
		break;

	case DBUS_TYPE_UINT32:
		uint32 = strtoul(value, NULL, 0);
		dbus_message_iter_append_basic(iter, DBUS_TYPE_UINT32, &uint32);
		break;

	case DBUS_TYPE_INT64:
		int64 = strtoll(value, NULL, 0);
		dbus_message_iter_append_basic(iter, DBUS_TYPE_INT64, &int64);
		break;

	case DBUS_TYPE_UINT64:
		uint64 = strtoull(value, NULL, 0);
		dbus_message_iter_append_basic(iter, DBUS_TYPE_UINT64, &uint64);
		break;

	case DBUS_TYPE_STRING:
		dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &value);
		break;

	case DBUS_TYPE_OBJECT_PATH:
		dbus_message_iter_append_basic(iter, DBUS_TYPE_OBJECT_PATH,
					       &value);
		break;

	case DBUS_TYPE_BOOLEAN:
		if (strcmp(value, "true") == 0) {
			v_BOOLEAN = TRUE;
			dbus_message_iter_append_basic(iter, DBUS_TYPE_BOOLEAN,
						       &v_BOOLEAN);
		} else if (strcmp(value, "false") == 0) {
			v_BOOLEAN = FALSE;
			dbus_message_iter_append_basic(iter, DBUS_TYPE_BOOLEAN,
						       &v_BOOLEAN);
		} else {
			fprintf(stderr,
				"FAIL: Expected \"true\" or \"false\" instead of \"%s\"\n",
				value);
			exit(1);
		}
		break;

	default:
		fprintf(stderr, "FAIL: Unsupported data type %c\n", (char)type);
		exit(1);
	}
}

void append_array(DBusMessageIter * iter, int type, const char *value)
{
	const char *val;
	char *dupval = strdup(value);

	val = strtok(dupval, ",");
	while (val != NULL) {
		append_arg(iter, type, val);
		val = strtok(NULL, ",");
	}
	free(dupval);
}

void
append_dict(DBusMessageIter * iter, int keytype, int valtype, const char *value)
{
	const char *val;
	char *dupval = strdup(value);

	val = strtok(dupval, ",");
	while (val != NULL) {
		DBusMessageIter subiter;

		dbus_message_iter_open_container(iter,
						 DBUS_TYPE_DICT_ENTRY,
						 NULL, &subiter);

		append_arg(&subiter, keytype, val);
		val = strtok(NULL, ",");
		if (val == NULL) {
			fprintf(stderr, "FAIL: Malformed dictionary\n");
			exit(1);
		}
		append_arg(&subiter, valtype, val);

		dbus_message_iter_close_container(iter, &subiter);
		val = strtok(NULL, ",");
	}
	free(dupval);
}

int type_from_name(const char *arg)
{
	int type;
	if (!strcmp(arg, "string"))
		type = DBUS_TYPE_STRING;
	else if (!strcmp(arg, "int16"))
		type = DBUS_TYPE_INT16;
	else if (!strcmp(arg, "uint16"))
		type = DBUS_TYPE_UINT16;
	else if (!strcmp(arg, "int32"))
		type = DBUS_TYPE_INT32;
	else if (!strcmp(arg, "uint32"))
		type = DBUS_TYPE_UINT32;
	else if (!strcmp(arg, "int64"))
		type = DBUS_TYPE_INT64;
	else if (!strcmp(arg, "uint64"))
		type = DBUS_TYPE_UINT64;
	else if (!strcmp(arg, "double"))
		type = DBUS_TYPE_DOUBLE;
	else if (!strcmp(arg, "byte"))
		type = DBUS_TYPE_BYTE;
	else if (!strcmp(arg, "boolean"))
		type = DBUS_TYPE_BOOLEAN;
	else if (!strcmp(arg, "objpath"))
		type = DBUS_TYPE_OBJECT_PATH;
	else {
		fprintf(stderr, "FAIL: Unknown type \"%s\"\n", arg);
		exit(1);
	}
	return type;
}
