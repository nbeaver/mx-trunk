/*
 * Name:     mx_osdef.h
 *
 * Purpose:  Header file for operating system dependent definitions.
 *
 * Author:   William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2003, 2005-2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef _MX_OSDEF_H_
#define _MX_OSDEF_H_

/* Define OS_UNIX if this is a significantly Unix-like operating system. */

#if defined(OS_LINUX) || defined(OS_AIX) || defined(OS_SOLARIS) \
 || defined(OS_SUNOS4) || defined(OS_IRIX) || defined(OS_HPUX) \
 || defined(OS_MACOSX) || defined(OS_BSD) || defined(OS_QNX) \
 || defined(OS_TRU64)

#  define OS_UNIX
#endif

/* Define OS_MSDOS if we are using an MSDOS-based system. */

#if defined(OS_DOS_EXT_WATCOM) || defined(OS_DJGPP)

#  define OS_MSDOS
#endif

/* ==== Operating system dependent parameters. ==== */

/* Do we have TCP/IP on this platform? */

#if defined(OS_MSDOS)
#  if defined(OS_DJGPP)
#     define HAVE_TCPIP			1
#  else
#     define HAVE_TCPIP			0
#  endif
#else
#  define HAVE_TCPIP			1
#endif

/* Do we have FD_SET() style macros for select() on this platform? */

#if (HAVE_TCPIP == 0)
#  define HAVE_FD_SET			0
#else
#  define HAVE_FD_SET			1
#endif

/* Do we have scatter-gather I/O available via readv() and writev()? */

#if defined( OS_WIN32 ) || defined( OS_DJGPP ) || defined( OS_RTEMS ) \
	|| defined( OS_ECOS )
#  define HAVE_READV_WRITEV		0
#else
#  define HAVE_READV_WRITEV		1
#endif

/* Do we have Unix domain sockets? */

#if defined( OS_UNIX )
#  define HAVE_UNIX_DOMAIN_SOCKETS	1
#else
#  define HAVE_UNIX_DOMAIN_SOCKETS	0
#endif

/* Do we have a version of FIONREAD that supports sockets? */

#if defined( OS_LINUX ) || defined( OS_MACOSX ) || defined( OS_SOLARIS )
#  define HAVE_FIONREAD_FOR_SOCKETS	1
#else
#  define HAVE_FIONREAD_FOR_SOCKETS	0
#endif

/* Do we have a version of FIONREAD that supports tty ports? */

#if 0 && defined( OS_LINUX )
#  define HAVE_FIONREAD_FOR_TTY_PORTS	1
#else
#  define HAVE_FIONREAD_FOR_TTY_PORTS	0
#endif

/* Do we have XDR data format support on this platform?
 *
 * XDR (eXternal Data Representation) is used by NFS and is part of the
 * Sun RPC, a.k.a. ONC-RPC package.
 */

#define HAVE_XDR			1

/* Do we have an implementation of getopt() for this platform?
 *
 * Note that MX comes with a bundled version of getopt() that probably
 * can be used if the operating system itself does not have one.
 */

#define HAVE_GETOPT			1

/* Do programs on this platform have a main() routine?  On some embedded
 * operating systems, they do not.
 */

#if defined(OS_RTEMS) || defined(OS_VXWORKS) || defined(OS_ECOS)
#  define HAVE_MAIN_ROUTINE		0
#else
#  define HAVE_MAIN_ROUTINE		1
#endif

#endif /* _MX_OSDEF_H_ */

