/*
 * Name:    e_cbflib.h
 *
 * Purpose: Header for the MX CBFlib extension.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2016 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __E_CBFLIB_H__
#define __E_CBFLIB_H__

/*----*/

typedef struct {
	MX_DYNAMIC_LIBRARY *cbflib_library;
} MX_CBFLIB_EXTENSION_PRIVATE;

extern MX_EXTENSION_FUNCTION_LIST mxext_cbflib_extension_function_list;

MX_API mx_status_type mxext_cbflib_initialize( MX_EXTENSION *extension );

MX_API mx_status_type mxext_cbflib_read_cbf_file( MX_IMAGE_FRAME **frame,
							char *datafile_name );

MX_API mx_status_type mxext_cbflib_write_cbf_file( MX_IMAGE_FRAME *frame,
							char *datafile_name );

#endif /* __E_CBFLIB_H__ */

