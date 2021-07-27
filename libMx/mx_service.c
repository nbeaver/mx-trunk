/* 
 * Name:    mx_service.c
 *
 * Purpose: MX function library for interacting with service control managers.
 *
 * Author:  William Lavender
 *
 * This set of functions is for interacting with platform-specific
 * service control managers.
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2019, 2021 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

#include "mx_util.h"
#include "mx_socket.h"
#include "mx_unistd.h"
#include "mx_service.h"

/* FIXME: At present, only Linux systemd is implemented. */

#if defined(OS_LINUX)

/* For Linux, we use "systemd".  This version is based on code from here
 *
 *    https://lists.debian.org/debian-ctte/2013/12/msg00230.html
 */

/* The following definition of MSG_NOSIGNAL is a kludge, since Linux versions
 * that do not define MSG_NOSIGNAL do not actually use systemd.
 */

#if !defined(MSG_NOSIGNAL)
#  define MSG_NOSIGNAL	0x4000
#endif

MX_EXPORT mx_status_type
mx_scm_notify( int notification_type,
		const char *message )
{
	const char *notify_socket_string = NULL;
	char scm_data[256];
	int fd;
	struct sockaddr_un un = { .sun_family = AF_UNIX };

	notify_socket_string = getenv("NOTIFY_SOCKET");

	if ( notify_socket_string == (const char *) NULL ) {

		/* We are not running under systemd, so we just return
		 * without doing anything else.
		 */

		return MX_SUCCESSFUL_RESULT;
	}

	/* FIXME: For now, we just implement MXSCM_RUNNING. */

	if ( notification_type != MXSCM_RUNNING ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* Construct the message to send to systemd. */

	if ( message == (const char *) NULL ) {
		strlcpy( scm_data, "READY=1\n", sizeof(scm_data) );
	} else {
		snprintf( scm_data, sizeof(scm_data),
				"READY=1\n%s", message );
	}

	strlcpy( un.sun_path, notify_socket_string, sizeof(un.sun_path) );

	if ( un.sun_path[0] == '@' ) {
		un.sun_path[0] = 0;
	}

	fd = socket( AF_UNIX, SOCK_DGRAM, 0 );

	if ( fd < 0 ) {
		/* FIXME: Print error message. */
		return MX_SUCCESSFUL_RESULT;
	}

	sendto( fd, scm_data, strlen(scm_data),
		MSG_NOSIGNAL,
		(struct sockaddr*) &un,
		offsetof(struct sockaddr_un, sun_path)
			+ strlen(notify_socket_string) );

	close( fd );

	return MX_SUCCESSFUL_RESULT;
}

#else

/* If this platform does not have a service control manager or we do not
 * want to use the native one, then we just use the following stub.
 */

MX_EXPORT mx_status_type
mx_scm_notify( int notification_type,
		const char *message )
{
	/* We do nothing here. */

	return MX_SUCCESSFUL_RESULT;
}

#endif

