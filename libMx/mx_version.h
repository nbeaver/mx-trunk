/*
 * Name:    mx_version.h
 *
 * Purpose: Header file for functions to display the version of MX being used.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_VERSION_H__
#define __MX_VERSION_H__

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

MX_API int mx_get_major_version( void );

MX_API int mx_get_minor_version( void );

MX_API int mx_get_update_version( void );

MX_API char *mx_get_version_date( void );

MX_API char *mx_get_version_string( void );

#ifdef __cplusplus
}
#endif

#endif /* __MX_VERSION_H__ */

