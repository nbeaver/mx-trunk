/*
 * Name:    mx_version.h
 *
 * Purpose: Support for cross-protocol network functionality.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2009 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_MULTI_H__
#define __MX_MULTI_H__

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

MX_API void mx_multi_set_debug_flag( MX_RECORD *record_list,
					mx_bool_type value );

#ifdef __cplusplus
}
#endif

#endif /* __MX_MULTI_H__ */

