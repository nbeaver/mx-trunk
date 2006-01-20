/*
 * Name:    mx_unistd.h
 *
 * Purpose: Not every platform has <unistd.h>, so we use this header file
 *          instead of including <unistd.h> directly.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2000-2001, 2003, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef _MX_UNISTD_H_
#define _MX_UNISTD_H_

#if defined(OS_WIN32)
#  include <io.h>

   unsigned int sleep( unsigned int seconds );

#else
#  include <unistd.h>
#endif

#if defined(OS_VXWORKS)
extern int access( char *pathname, int mode );
#endif

#if defined(OS_WIN32) || defined(OS_VXWORKS)
#  define R_OK  4
#  define W_OK  2
#  define X_OK  1
#  define F_OK  0
#endif

#if defined(OS_CYGWIN)
#  include <getopt.h>
#else
#  if defined(OS_WIN32) || defined(OS_SUNOS4) || defined(OS_VXWORKS)
      MX_API int getopt(int argc, char *argv[], char *optstring);
#  endif

   MX_API char *optarg;
   MX_API int optind;
#endif

#endif /* _MX_UNISTD_H_ */

