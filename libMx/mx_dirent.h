/*
 * Name:    mx_dirent.h
 *
 * Purpose: Posix style directory stream functions.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef _MX_DIRENT_H_
#define _MX_DIRENT_H_

#if defined(OS_WIN32)
#error WIN32 support not yet implemented for mx_dirent.h
#else
#  include <dirent.h>
#endif

#endif /* _MX_DIRENT_H_ */

