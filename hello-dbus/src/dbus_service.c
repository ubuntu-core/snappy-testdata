/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */
/* dbus_service.c  Utility program to send messages from the command line
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
#include <signal.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "dbus_common.h"

static int terminate = 0;
DBusConnection *connection = NULL;
DBusError error;
DBusBusType type = DBUS_BUS_SESSION;
const char *name = NULL;
const char *path = NULL;
const char *interface = NULL;
const char *member = NULL;
const char *address = NULL;
int session_or_system = FALSE;
int log_fd = -1;
int lock_fd = 0;

static void usage(void)
{
	fprintf(stderr,
		"Usage: dbus_service [ADDRESS] --name=<NAME> <path> <interface>\n\n"
		"    ADDRESS\t\t--system, --session (default), or --address=ADDR\n"
		"    NAME\t\tthe well-known name to bind to\n"
		"    path\t\tpath to object (such as /org/freedesktop/DBus)\n"
		"    interface\t\tinterface to use (such as org.freedesktop.DBus)\n\n"
		"    The method <interface>.Method replies with an empty method_reply message.\n"
		"    The signal <interface>.Signal is accepted by the service.\n");
}

/**
 * Returns -1 upon error, 0 when there are no more messages
 */
static int handle_messages(void)
{
	DBusMessage *message;

	if (!dbus_connection_read_write(connection, 250)) {
		fprintf(stderr, "FAIL: Connecion is closed\n");
		return -1;
	}

	for (;;) {
		message = dbus_connection_pop_message(connection);
		if (message == NULL)
			return 0;

		log_message(log_fd, "received ", message);

		if (dbus_message_is_signal(message, interface, "Signal")) {
			dbus_message_unref(message);
			continue;
		} else
		    if (dbus_message_is_method_call
			(message, interface, "Method")) {
			DBusMessage *reply;

			reply = dbus_message_new_method_return(message);
			dbus_message_unref(message);

			log_message(log_fd, "sent ", reply);
			dbus_connection_send(connection, reply, NULL);
			dbus_connection_flush(connection);
			dbus_message_unref(reply);
			continue;
		} else if (dbus_message_get_type(message) ==
			   DBUS_MESSAGE_TYPE_METHOD_CALL) {
			DBusMessage *reply;

			reply =
			    dbus_message_new_error(message,
						   DBUS_ERROR_UNKNOWN_METHOD,
						   NULL);
			dbus_message_unref(message);

			log_message(log_fd, "sent ", reply);
			dbus_connection_send(connection, reply, NULL);
			dbus_connection_flush(connection);
			dbus_message_unref(reply);
			continue;
		} else {
			dbus_message_unref(message);
			continue;
		}
	}

	return 0;
}

void sigterm_handler(int signum)
{
	terminate = 1;
}

static int setup_signal_handling(void)
{
	struct sigaction sa;
	int rc;

	sa.sa_handler = sigterm_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	rc = sigaction(SIGTERM, &sa, NULL);
	if (rc < 0) {
		fprintf(stderr, "FAIL: Could not set up signal handling\n");
		return 1;
	}

	return 0;
}

static int unlock_fd(void)
{
	int rc;

	if (lock_fd < 0)
		return 0;

	rc = flock(lock_fd, LOCK_UN);
	if (rc < 0)
		fprintf(stderr, "FAIL: Failed to unlock lock file: %m\n");

	return rc;
}

static int do_service(void)
{
	int rc;

	rc = dbus_bus_request_name(connection, name,
				   DBUS_NAME_FLAG_REPLACE_EXISTING, &error);
	if (dbus_error_is_set(&error)) {
		fprintf(stderr, "FAIL: %s: %s\n", error.name, error.message);
		dbus_error_free(&error);
	}
	if (rc != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
		return 1;
	}

	if (unlock_fd())
		return 1;

	rc = 0;
	while (!terminate && !rc)
		rc = handle_messages();

	/* If we've received SIGTERM, try one last time to drain the incoming queue */
	if (terminate && !rc)
		rc = handle_messages();

	if (rc < 0)
		return 1;

	rc = dbus_bus_release_name(connection, name, &error);
	if (dbus_error_is_set(&error)) {
		fprintf(stderr, "FAIL: %s: %s\n", error.name, error.message);
		dbus_error_free(&error);
	}
	if (rc != DBUS_RELEASE_NAME_REPLY_RELEASED) {
		return 1;
	}

	return 0;
}

int main(int argc, char *argv[])
{
	int i, rc;

	if (argc < 3) {
		usage();
		rc = 1;
		goto out;
	}

	rc = setup_signal_handling();
	if (rc != 0) {
		fprintf(stderr, "FAIL: Couldn't set up signal handler\n");
		rc = 1;
		goto out;
	}

	for (i = 1; i < argc && interface == NULL; i++) {
		char *arg = argv[i];

		if (strcmp(arg, "--system") == 0) {
			type = DBUS_BUS_SYSTEM;
			session_or_system = TRUE;
		} else if (strcmp(arg, "--session") == 0) {
			type = DBUS_BUS_SESSION;
			session_or_system = TRUE;
		} else if (strstr(arg, "--address") == arg) {
			address = strchr(arg, '=');

			if (address == NULL) {
				fprintf(stderr,
					"FAIL: \"--address=\" requires an ADDRESS\n");
				usage();
				rc = 1;
				goto out;
			} else {
				address = address + 1;
			}
		} else if (strstr(arg, "--name=") == arg)
			name = strchr(arg, '=') + 1;
		else if (strstr(arg, "--log=") == arg) {
			char *path = strchr(arg, '=') + 1;

			log_fd = open(path, O_CREAT | O_TRUNC | O_WRONLY,
				      S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP |
				      S_IROTH | S_IWOTH);
			if (log_fd < 0) {
				fprintf(stderr,
					"FAIL: Couldn't open log file \"%s\"\n",
					path);
				exit(1);
			}
		} else if (strstr(arg, "--lock-fd=") == arg) {
			char *fd = strchr(arg, '=') + 1;

			lock_fd = atoi(fd);
		} else if (!strcmp(arg, "--help")) {
			usage();
			rc = 0;
			goto out;
		} else if (arg[0] == '-') {
			usage();
			rc = 1;
			goto out;
		} else if (path == NULL)
			path = arg;
		else	/* interface == NULL guaranteed by the 'while' loop */
			interface = arg;
	}

	if (name == NULL || path == NULL || interface == NULL || i < argc) {
		usage();
		rc = 1;
		goto out;
	}

	if (session_or_system && (address != NULL)) {
		fprintf(stderr,
			"FAIL: \"--address\" may not be used with \"--system\" or \"--session\"\n");
		usage();
		rc = 1;
		goto out;
	}

	dbus_error_init(&error);

	if (address != NULL)
		connection = dbus_connection_open(address, &error);
	else
		connection = dbus_bus_get(type, &error);

	if (connection == NULL) {
		fprintf(stderr,
			"FAIL: Failed to open connection to \"%s\" message bus: %s\n",
			address ? address :
				  ((type == DBUS_BUS_SYSTEM) ? "system" : "session"),
			error.message);
		dbus_error_free(&error);
		rc = 1;
		goto out;
	} else if (address != NULL)
		dbus_bus_register(connection, &error);

	rc = do_service();

out:
	if (connection)
		dbus_connection_unref(connection);

	unlock_fd();

	if (rc == 0)
		printf("PASS\n");

	exit(rc);
}
