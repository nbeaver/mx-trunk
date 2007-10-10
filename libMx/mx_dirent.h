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

#include <windows.h>

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	HANDLE find_handle;
	WIN32_FIND_DATA find_data;
	long file_number;
	char directory_name[MAX_PATH+1];
} DIR;

struct dirent {
	char d_name[MAX_PATH+1];
};

MX_API DIR *opendir( const char *name );

MX_API int closedir( DIR *dir );

MX_API struct dirent *readdir( DIR *dir );

MX_API void rewinddir( DIR *dir );

#ifdef __cplusplus
}
#endif

#else  /* not OS_WIN32 */

#  include <dirent.h>
#endif

#endif /* _MX_DIRENT_H_ */

