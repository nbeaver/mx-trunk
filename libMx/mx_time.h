/*
 * Name:    mx_time.h
 *
 * Purpose: This is a wrapper for <time.h> that defines thread-safe Posix
 *          time functions for those platforms that do not have them and
 *          also adds some MX-specific functions.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef _MX_TIME_H_
#define _MX_TIME_H_

#include <time.h>

#if defined(OS_WIN32) || defined(OS_DJGPP)

  MX_API char *asctime_r( const struct tm *, char * );

  MX_API char *ctime_r( const time_t *, char * );

  MX_API struct tm *gmtime_r( const time_t *, struct tm * );

  MX_API struct tm *localtime_r( const time_t *, struct tm * );

#endif

/*---- MX OS time reporting functions. ----*/

MX_API struct timespec mx_current_os_time( void );

MX_API char *mx_os_time_string( struct timespec os_time,
				char *buffer, size_t buffer_length );

MX_API char *mx_ctime_string( void );

MX_API char *mx_current_time_string( char *buffer, size_t buffer_length );

/*----*/

#endif /* _MX_TIME_H_ */
