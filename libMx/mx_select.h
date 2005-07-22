/*
 * Name:    mx_select.h
 *
 * Purpose: Function prototype for select().
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2003 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_SELECT_H__
#define __MX_SELECT_H__

#if defined(OS_AIX)

#   include <sys/select.h>

#elif defined(OS_IRIX)

#   include <sys/time.h>

#elif defined(OS_MACOSX)

#   include <unistd.h>

#elif defined(OS_BSD)

#   include <sys/types.h>
#   include <sys/time.h>
#   include <unistd.h>

#elif defined(OS_SUNOS4)

#   include <sys/types.h>
#   include <sys/time.h>

    extern int select( int, fd_set *, fd_set *, fd_set *, struct timeval * );

#elif defined(OS_QNX)

#   include <sys/select.h>

#elif defined(OS_WIN32)

#   include <winsock.h>

#else

    extern int select( int, fd_set *, fd_set *, fd_set *, struct timeval * );

#endif

#endif /* __MX_SELECT_H__ */

