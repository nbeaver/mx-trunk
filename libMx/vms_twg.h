/*
 * Name:    vms_twg.h
 *
 * Purpose: Helper definitions for The Wollogong Group's WIN/TCP for VMS.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999, 2001 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 *---------------------------------------------------------------------------
 *
 * Parts of this header were copied from BSD derived header files in WIN/TCP
 * and are covered by the following copyright:
 *
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef _VMS_TWG_H_
#define _VMS_TWG_H_

/* I am not entirely sure as to what the 'noshare' keyword does, so I hope
 * that I can get away with defining it to be an empty string.
 */

#define noshare

#include <sys/types.h>

#include "[networks.netdist.include.sys]time.h"
#include "[networks.netdist.include.sys]socket.h"
#include "[networks.netdist.include.netinet]in.h"
#include "[networks.netdist.include.netns]ns.h"
#include "[networks.netdist.include.arpa]inet.h"
#include "[networks.netdist.include]netdb.h"

#ifndef FD_SETSIZE
#define FD_SETSIZE	256
#endif

typedef long	fd_mask;

#define NFDBITS	(sizeof(fd_mask) * NBBY)	/* bits per mask */

typedef	struct fd_set {
	fd_mask	fds_bits[64];
} fd_set;

#define	FD_SET(n, p)	((p)->fds_bits[(n)/NFDBITS] |= (1 << ((n) % NFDBITS)))
#define	FD_CLR(n, p)	((p)->fds_bits[(n)/NFDBITS] &= ~(1 << ((n) % NFDBITS)))
#define	FD_ISSET(n, p)	((p)->fds_bits[(n)/NFDBITS] & (1 << ((n) % NFDBITS)))
#define FD_ZERO(p)	fdzero(p)

extern void fdzero( fd_set *p );

/* WIN/TCP does not have prototypes for a lot of its functions, so
 * we must do it ourself.  The prototypes are written to match what
 * is listed in the WIN/TCP Programming Guide.
 */

#define vms_close	netclose

extern int socket( int domain, int type, int protocol );

extern int connect( int fd, struct sockaddr *name, int namelen );

extern int bind( int fd, struct sockaddr *name, int namelen );

extern int listen( int fd, int backlog );

extern int send( int fd, char *message, int length, int flags );

extern int recv( int fd, char *message, int length, int flags );

extern int select( int nfds, fd_set *readfds, fd_set *write_fds,
	fd_set *exceptfds, struct timeval *timeout );

extern int ioctl( int fd, unsigned long request, ... );

extern int setsockopt( int fd, int level, int optname,
				char *optval, int optlen );

extern int shutdown( int fd, int how );

extern int netclose( int fd );

/* Finally, <sys/ioctl.h> for WIN/TCP has includes for other files in
 * the sys directory.  GCC objects to these include statements since
 * they are made with <>, but the files are not in a directory in
 * GCC's search path for include files.  Instead, we merely copy here 
 * the small part of <sys/ioctl.h> that we are actually using.
 */

/*
 * Ioctl's have the command encoded in the lower word,
 * and the size of any in or out parameters in the upper
 * word.  The high 2 bits of the upper word are used
 * to encode the in/out status of the parameter; for now
 * we restrict parameters to at most 128 bytes.
 */
#define	IOCPARM_MASK	0x7f		/* parameters must be < 128 bytes */
#define	IOC_VOID	0x20000000	/* no parameters */
#define	IOC_OUT		0x40000000	/* copy out parameters */
#define	IOC_IN		0x80000000	/* copy in parameters */
#define	IOC_INOUT	(IOC_IN|IOC_OUT)
/* the 0x20000000 is so we can distinguish new ioctl's from old */
#define	_IO(x,y)	(IOC_VOID|('x'<<8)|y)
#define	_IOR(x,y,t)	(IOC_OUT|((sizeof(t)&IOCPARM_MASK)<<16)|('x'<<8)|y)
#define	_IOW(x,y,t)	(IOC_IN|((sizeof(t)&IOCPARM_MASK)<<16)|('x'<<8)|y)
/* this should be _IORW, but stdio got there first */
#define	_IOWR(x,y,t)	(IOC_INOUT|((sizeof(t)&IOCPARM_MASK)<<16)|('x'<<8)|y)

#define	FIONBIO		_IOW(f, 126, int)	/* set/clear non-blocking i/o */

#endif /* _VMS_TWG_H_ */

