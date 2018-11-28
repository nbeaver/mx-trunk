/*
 * Name:    e_libcurl.h
 *
 * Purpose: Header for the MX libcurl extension.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2018 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __E_LIBCURL_H__
#define __E_LIBCURL_H__

/*----*/

typedef struct {
	MX_DYNAMIC_LIBRARY *libcurl_library;
} MX_LIBCURL_EXTENSION_PRIVATE;

extern MX_EXTENSION_FUNCTION_LIST mxext_libcurl_extension_function_list;

MX_API mx_status_type mxext_libcurl_initialize( MX_EXTENSION *extension );


#endif /* __E_LIBCURL_H__ */

