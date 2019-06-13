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
 * WARNING: The implementations below of mx_getaddrinfo() and mx_getnameinfo()
 *          are not thread-safe, since they rely on underlying functions like
 *          gethostbyname(), gethostbyaddr(), etc. that are not thread-safe.
 *
 *          There _do_ exist platforms that add reentrant versions of the
 *          underlying functions such as gethostbyname_r() and friends.
 *          However, such platforms tend to have _real_ versions of 
 *          getaddrinfo() and getnameinfo(), so gethostbyname_r() and
 *          friends are probably irrelevant here.
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

#if !defined(OS_VXWORKS)

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "mx_util.h"
#include "mx_socket.h"
#include "mx_netdb.h"

/* Note: gethostbyname(), getprotobyname(), and getservbyname() are not
 *       thread-safe.
 */

MX_EXPORT int
mx_getaddrinfo( const char *node,
		const char *service,
		const struct addrinfo *hints,
		struct addrinfo **res )
{
	struct hostent *hostent_ptr = NULL;
	struct protoent *protoent_ptr = NULL;
	struct servent *servent_ptr = NULL;

	struct addrinfo *new_addrinfo = NULL;
	struct sockaddr_in *new_sockaddr_in = NULL;

	errno = 0;

	if ( node == (const char *) NULL ) {
		errno = EINVAL;
		return EAI_SYSTEM;
	}

	/* Allocate memory for 'new_addrinfo' before trying to do the lookup. */

	new_addrinfo = calloc( 1, sizeof(struct addrinfo) );

	if ( new_addrinfo == (struct addrinfo *) NULL ) {
		return EAI_MEMORY;
	}

	new_sockaddr_in = calloc( 1, sizeof(struct sockaddr_in) );

	if ( new_sockaddr_in == (struct sockaddr_in *) NULL ) {
		return EAI_MEMORY;
	}

	/*---*/

	servent_ptr = getservbyname( service, NULL );

	if ( servent_ptr == (struct servent *) NULL ) {
		return EAI_FAIL;
	}

	/*---*/

	protoent_ptr = getprotobyname( servent_ptr->s_proto );

	if ( protoent_ptr == (struct protoent *) NULL ) {
		return EAI_FAIL;
	}

	new_addrinfo->ai_protocol = protoent_ptr->p_proto;

	new_sockaddr_in->sin_port = servent_ptr->s_port;

	/*---*/

	hostent_ptr = gethostbyname( node );

	if ( hostent_ptr == (struct hostent *) NULL ) {
		return EAI_FAIL;
	}

	new_sockaddr_in->sin_addr.s_addr = inet_addr( hostent_ptr->h_addr );
	new_addrinfo->ai_canonname = strdup( hostent_ptr->h_name );

	/* If present, examine the requested values in the 'hints' structure. */

	if ( hints != (struct addrinfo *) NULL ) {
		if ( hints->ai_family != AF_INET ) {
			return EAI_FAMILY;
		}

		switch( hints->ai_socktype ) {
		case 0:
			new_addrinfo->ai_socktype = SOCK_STREAM;
			break;
		case SOCK_STREAM:
		case SOCK_DGRAM:
			new_addrinfo->ai_socktype = hints->ai_socktype;
			break;
		default:
			return EAI_SOCKTYPE;
			break;
		}
	}

	/* Only IPv4 is supported here. */

	new_addrinfo->ai_flags = 0x0;
	new_addrinfo->ai_family = AF_INET;
	new_sockaddr_in->sin_family = AF_INET;

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

/* Note: gethostbyaddr() and getservbyport() are not thread-safe. */

MX_EXPORT int
mx_getnameinfo( const struct sockaddr *sa,
		mx_socklen_t salen,
		char *host,
		mx_socklen_t hostlen,
		char *serv,
		mx_socklen_t servlen,
		int flags )
{
	const struct sockaddr_in *sa_in = NULL;
	struct hostent *hostent_ptr = NULL;
	struct servent *servent_ptr = NULL;
	size_t host_name_length, service_name_length;

	errno = 0;

	if ( sa == (struct sockaddr *) NULL ) {
		errno = EINVAL;
		return EAI_SYSTEM;
	}

	/* The size of the 'struct sockaddr' needs to be at least as big
	 * as a 'struct sockaddr_in', since only AF_INET is supported.
	 */

	if ( salen < sizeof(struct sockaddr_in) ) {
		return EAI_OVERFLOW;
	}

	/* Once again, only IPv4 is supported here. */

	/* The (const void *) cast is to make clang on macOS happy. */

	sa_in = (const void *) sa;

	if ( sa_in->sin_family != AF_INET ) {
		return EAI_FAMILY;
	}

	/* Get the host name for this address. */

	hostent_ptr = gethostbyaddr( (const void *) &(sa_in->sin_addr),
				sizeof(struct in_addr), AF_INET );

	if ( hostent_ptr == (struct hostent *) NULL ) {
		return EAI_FAIL;
	}

	host_name_length = strlcpy( host, hostent_ptr->h_name, hostlen );

	if ( host_name_length >= hostlen ) {
		return EAI_OVERFLOW;
	}

	/* Get the service name for this address. */

	servent_ptr = getservbyport( sa_in->sin_port, NULL );

	if ( servent_ptr == (struct servent *) NULL ) {
		return EAI_FAIL;
	}

	service_name_length = strlcpy( serv, servent_ptr->s_name, servlen );

	if ( service_name_length >= servlen ) {
		return EAI_OVERFLOW;
	}

	return 0;
}

#endif
