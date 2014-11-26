/*
 * Name:    mx_version.h
 *
 * Purpose: Functions and macro definitions that report the version of MX
 *          in use, as well as other definitions for the version of the
 *          compiler or runtime libraries in use.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2007-2008, 2014 Illinois Institute of Technology
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

#include "mx_private_version.h"

MX_API int mx_get_major_version( void );

MX_API int mx_get_minor_version( void );

MX_API int mx_get_update_version( void );

MX_API char *mx_get_version_full_string( void );

MX_API char *mx_get_version_date_string( void );

MX_API struct tm mx_get_version_date_tm( void );

MX_API time_t mx_get_version_date_time( void );

#ifdef __cplusplus
}
#endif

#endif /* __MX_VERSION_H__ */

