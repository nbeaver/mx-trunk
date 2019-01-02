/*
 * Name:    mx_netdb.h 
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
 *----------------------------------------------------------------------
 *
 * Copyright 2018-2019 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_NETDB_H__
#define __MX_NETDB_H__

#include "mx_socket.h"

#if defined( _NETDB_H )
#  define __MX_NEED_ADDRINFO	FALSE

#elif defined( _NETDB_H_ )
#  define __MX_NEED_ADDRINFO	FALSE

#elif defined( OS_ANDROID )
#  define __MX_NEED_ADDRINFO	FALSE
#else
#  define __MX_NEED_ADDRINFO	TRUE
#endif

/*---*/

#if !defined( PF_UNSPEC )
#  define PF_UNSPEC	0
#endif

#if !defined( PF_INET6 )
#  define PF_INET6	10
#endif

#if !defined( AF_UNSPEC )
#  define AF_UNSPEC	0
#endif

#if !defined( AF_INET6 )
#  define AF_INET6	10
#endif

#if !defined( EAI_SYSTEM )
#  define EAI_SYSTEM    -11
#endif

/*---*/

#if __MX_NEED_ADDRINFO

  struct addrinfo {
    int              ai_flags;
    int              ai_family;
    int              ai_socktype;
    int              ai_protocol;
    mx_socklen_t     ai_addrlen;
    struct sockaddr *ai_addr;
    char            *ai_canonname;
    struct addrinfo *ai_next;
  };

#endif

/*----*/

MX_API int
mx_getaddrinfo( const char *node,
		const char *service,
		const struct addrinfo *hints,
		struct addrinfo **res );

MX_API void
mx_freeaddrinfo( struct addrinfo *res );

MX_API const char *
mx_gai_strerror( int errcode );

/*----*/

/* flags argument for mx_getnameinfo(). */

#if !defined( NI_NAMEREQD )
#  define NI_NUMERICHOST 0x1
#  define NI_NUMERICSERV 0x2
#  define NI_NOFQDN      0x4
#  define NI_NAMEREQD    0x8
#  define NI_DGRAM       0x10
#endif

MX_API int
mx_getnameinfo( const struct sockaddr *sa,
		mx_socklen_t salen,
		char *host,
		mx_socklen_t hostlen,
		char *serv,
		mx_socklen_t servlen,
		int flags );

#if 0
#  define getaddrinfo( n, s, h, r )	mx_getaddrinfo( n, s, h, r )

#  define getnameinfo( s, sl, h, hl, r, rl, f ) \
		mx_getnameinfo( s, sl, h, hl, r, rl, f )

#  define freeaddrinfo( r )	mx_freeaddrinfo( r )

#  define gai_strerror( e )	mx_gai_strerror( e )
#endif

#endif /* __MX_NETDB_H__ */
