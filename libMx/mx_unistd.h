/*
 * Name:    mx_unistd.h
 *
 * Purpose: Not every platform has <unistd.h>, so we use this header file
 *          instead of including <unistd.h> directly.  This header can also
 *          be used to find functions like mkdir() and stat().
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2000-2001, 2003, 2006-2007, 2009 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef _MX_UNISTD_H_
#define _MX_UNISTD_H_

#if defined(OS_WIN32)
#  include <io.h>

   /* Make the definition of sleep() C++ safe. */

#  ifdef __cplusplus
   extern "C" {
#  endif

   unsigned int sleep( unsigned int seconds );

#  ifdef __cplusplus
   }
#  endif

#else
#  include <stdlib.h>
#  include <sys/stat.h>
#  include <sys/types.h>
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

/*----*/

/* Some platforms define mkdir() differently or not at all, so we provide
 * our own front end here.
 */

#if defined(OS_VXWORKS) || defined(OS_WIN32)

#  include <sys/types.h>

#  if defined(_MSC_VER)
      typedef unsigned int mode_t;
#  endif

#  define mkdir(p,m)	mx_mkdir((p),(m))

   /* Make the definition of mx_mkdir() C++ safe. */

#  ifdef __cplusplus
   extern "C" {
#  endif

   MX_API int mx_mkdir( char *pathname, mode_t mode );

#  ifdef __cplusplus
   }
#  endif
#endif

/*----*/

/* Temporary file creation. */

#if defined(OS_WIN32)

#  ifdef __cplusplus
   extern "C" {
#  endif

   MX_API int mkstemp( char *template_string );

#  ifdef __cplusplus
   }
#  endif
#endif

/*----*/

#if defined(OS_CYGWIN)
#  include <getopt.h>
#else
#  if defined(OS_WIN32) || defined(OS_SUNOS4) || defined(OS_VXWORKS) \
	|| defined(OS_ECOS)

      /* Make the definition of getopt() C++ safe. */

#     ifdef __cplusplus
      extern "C" {
#     endif

      MX_API int getopt(int argc, char *argv[], char *optstring);

#     if !defined(__MX_LIBRARY__)
         MX_API char *optarg;
         MX_API int   optind;
#     endif

#     ifdef __cplusplus
      }
#     endif
#  endif

#endif

#endif /* _MX_UNISTD_H_ */

