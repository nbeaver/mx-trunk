/*
 * Name:    e_libtiff.h
 *
 * Purpose: Header for the MX libtiff extension.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __E_LIBTIFF_H__
#define __E_LIBTIFF_H__

/*----*/

typedef struct {
	int dummy;
} MX_LIBTIFF_EXTENSION_PRIVATE;

extern MX_EXTENSION_FUNCTION_LIST mxext_libtiff_extension_function_list;

MX_API mx_status_type mxext_libtiff_initialize( MX_EXTENSION *extension );

MX_API mx_status_type mxext_libtiff_finalize( MX_EXTENSION *extension );

#endif /* __E_LIBTIFF_H__ */

