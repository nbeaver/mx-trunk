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
 * Copyright 2018-2019, 2021 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_NETDB_H__
#define __MX_NETDB_H__

#include "mx_socket.h"

#if defined( OS_CYGWIN )
#  include "cygwin/version.h"

#  if ( CYGWIN_VERSION_DLL_COMBINED >= 2000000 )
#    define __MX_NEED_ADDRINFO	FALSE
#  else
#    define __MX_NEED_ADDRINFO	TRUE
#  endif

#elif ( defined( _NETDB_H ) || defined( _NETDB_H_ ) || defined( OS_ANDROID ) \
	|| defined( OS_VXWORKS ) || defined( OS_QNX ) )
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

#if !defined( EAI_BADFLAGS )
#  define EAI_BADFLAGS	-1
#  define EAI_NONAME	-2
#  define EAI_AGAIN	-3
#  define EAI_FAIL	-4
#  define EAI_FAMILY	-6
#  define EAI_SOCKTYPE	-7
#  define EAI_SERVICE	-8
#  define EAI_MEMORY	-10
#  define EAI_SYSTEM    -11
#  define EAI_OVERFLOW	-12
#endif

/* The following are GNU extensions. */

#if !defined( EAI_NODATA )
#  define EAI_NODATA		-5
#  define EAI_ADDRFAMILY	-9
#  define EAI_INPROGRESS	-100
#  define EAI_CANCELED		-101
#  define EAI_NOTCANCELED	-102
#  define EAI_ALLDONE		-103
#  define EAI_INTR		-104
#  define EAI_IDN_ENCODE	-105
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
