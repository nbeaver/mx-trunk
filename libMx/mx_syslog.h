/*
 * Name:     mx_syslog.h
 *
 * Purpose:  Header file for Unix style syslogd daemon support.
 *
 * Author:   William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2000-2001, 2003, 2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_SYSLOG_H__
#define __MX_SYSLOG_H__

#if defined(OS_UNIX)

#  define MX_SYSLOG_IS_AVAILABLE	TRUE

#  include <syslog.h>

#elif defined(OS_VMS)

#  define MX_SYSLOG_IS_AVAILABLE	TRUE

   /* Make the definition of the syslog functions C++ safe. */

   #ifdef __cplusplus
   extern "C" {
   #endif

   extern void syslog( int, const char *, ... );
   extern void openlog( const char *, int, int );
   extern void closelog( void );

   #ifdef __cplusplus
   }
   #endif

#  define LOG_EMERG	0
#  define LOG_ALERT	1
#  define LOG_CRIT	2
#  define LOG_ERR	3
#  define LOG_WARNING	4
#  define LOG_NOTICE	5
#  define LOG_INFO	6
#  define LOG_DEBUG	7

#  define LOG_LOCAL0	(16<<3)
#  define LOG_LOCAL1	(17<<3)
#  define LOG_LOCAL2	(18<<3)
#  define LOG_LOCAL3	(19<<3)
#  define LOG_LOCAL4	(20<<3)
#  define LOG_LOCAL5	(21<<3)
#  define LOG_LOCAL6	(22<<3)
#  define LOG_LOCAL7	(23<<3)

#  define LOG_PID	0x01

#else
#  define MX_SYSLOG_IS_AVAILABLE	FALSE
#endif

#define MXF_SYSLOG_USE_STDERR	1

/* Make the definition of mx_install_syslog_handerl() C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

MX_API mx_status_type mx_install_syslog_handler( char *ident_string,
						int log_number,
						int log_options );
#ifdef __cplusplus
}
#endif

#endif /* __MX_SYSLOG_H__ */

