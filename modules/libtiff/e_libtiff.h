/*
 * Name:    e_libtiff.h
 *
 * Purpose: Header for the MX libtiff extension.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2015, 2019 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __E_LIBTIFF_H__
#define __E_LIBTIFF_H__

/*----*/

typedef struct {
	MX_DYNAMIC_LIBRARY *libtiff_library;
} MX_LIBTIFF_EXTENSION_PRIVATE;

extern MX_EXTENSION_FUNCTION_LIST mxext_libtiff_extension_function_list;

MX_API mx_status_type mxext_libtiff_initialize( MX_EXTENSION *extension );

MX_API mx_status_type mxext_libtiff_read_tiff_file( MX_IMAGE_FRAME **frame,
							char *image_filename );

MX_API mx_status_type mxext_libtiff_write_tiff_file( MX_IMAGE_FRAME *frame,
							char *image_filename );

MX_API mx_status_type mxext_libtiff_read_tiff_array( MX_IMAGE_FRAME **frame,
							long *image_size,
							void *image_array );

#endif /* __E_LIBTIFF_H__ */

