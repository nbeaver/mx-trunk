/*
 * Name:    mx_netdb.c
 *
 * Purpose: Implements getaddrinfo() and getnameinfo() using the older
 *          gethostbyname() and gethostbyaddr() functions.
 *
 * Author:  William Lavender
 *
 * Some old build targets do not come with native versions of getaddrinfo()
 * and getnameinfo().  This interface provides versions of those function
 * built on top of the old, deprecated gethostbyname() and gethostbyaddr()
 * functions.  Of course, these versions only provide support for IPV4.
 *
 * WARNING: The functions in this file are not yet tested!!!  Do not
 *          use them just yet.
 *
 *----------------------------------------------------------------------
 *
 * Copyright 2018-2019 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "mx_util.h"
#include "mx_socket.h"
#include "mx_netdb.h"

MX_EXPORT int
mx_getaddrinfo( const char *node,
		const char *service,
		const struct addrinfo *hints,
		struct addrinfo **res )
{
	struct hostent *hostent_ptr = NULL;
	struct servent *servent_ptr = NULL;

	struct addrinfo *new_addrinfo = NULL;
	struct sockaddr_in *new_sockaddr_in = NULL;

	if ( node == (const char *) NULL ) {
		errno = EINVAL;
		return EAI_SYSTEM;
	}

	/* FIXME: At present, we do not support the hints structure. */

	if ( hints != (struct addrinfo *) NULL ) {
		errno = EINVAL;
		return EAI_SYSTEM;
	}

	/* Allocate memory for 'new_addrinfo' before trying to do the lookup. */

	new_addrinfo = calloc( 1, sizeof(struct addrinfo) );

	if ( new_addrinfo == (struct addrinfo *) NULL ) {
		errno = ENOMEM;
		return EAI_SYSTEM;
	}

	new_sockaddr_in = calloc( 1, sizeof(struct sockaddr_in) );

	if ( new_sockaddr_in == (struct sockaddr_in *) NULL ) {
		errno = ENOMEM;
		return EAI_SYSTEM;
	}


	/*---*/

	/* FIXME: Lock getservbyname() mutex?  But this function is not
	 * necessarily the only user of getservbyname().
	 */

	servent_ptr = getservbyname( service, NULL );

	if ( servent_ptr == (struct servent *) NULL ) {
		errno = EINVAL;
		return EAI_SYSTEM;
	}

	/* FIXME: Unlock getservbyname() mutex? */

	/*---*/

	/* FIXME: Lock gethostbyname() mutex?  But this function is not
	 * necessarily the only user of gethostbyname().
	 */

	hostent_ptr = gethostbyname( node );

	if ( hostent_ptr == (struct hostent *) NULL ) {
		errno = EINVAL;
		return EAI_SYSTEM;
	}

	new_sockaddr_in->sin_addr.s_addr = inet_addr( hostent_ptr->h_addr );
	new_addrinfo->ai_canonname = strdup( hostent_ptr->h_name );

	/* FIXME: Unlock gethostbyname() mutex? */

	new_sockaddr_in->sin_family = AF_INET;
	new_sockaddr_in->sin_port = servent_ptr->s_port;

	/* Only IPv4 is supported here. */

	new_addrinfo->ai_flags = 0x0;
	new_addrinfo->ai_family = AF_INET;

	/* FIXME: We hardcode the following for now. */

	new_addrinfo->ai_socktype = SOCK_STREAM;
	new_addrinfo->ai_protocol = IPPROTO_TCP;

	new_addrinfo->ai_addrlen = sizeof(struct sockaddr_in);

	new_addrinfo->ai_addr = (struct sockaddr *) new_sockaddr_in;

	new_addrinfo->ai_next = NULL;

	*res = new_addrinfo;

	return 0;
}

MX_EXPORT void
mx_freeaddrinfo( struct addrinfo *res )
{
	if ( res == (struct addrinfo *) NULL ) {
		return;
	}

	mx_free( res->ai_addr );
	mx_free( res->ai_canonname );
	mx_free( res );

	return;
}

MX_EXPORT const char *
mx_gai_strerror( int foo )
{
	return NULL;
}

MX_EXPORT int
mx_getnameinfo( const struct sockaddr *sa,
		mx_socklen_t salen,
		char *host,
		mx_socklen_t hostlen,
		char *serv,
		mx_socklen_t servlen,
		int flags )
{
	return (-1);
}

