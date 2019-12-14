/*
 * Name:    e_hdf5.h
 *
 * Purpose: Header for the MX HDF5 extension.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2019 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __E_HDF5_H__
#define __E_HDF5_H__

/*----*/

typedef struct {
	MX_DYNAMIC_LIBRARY *hdf5_library;
} MX_HDF5_EXTENSION_PRIVATE;

extern MX_EXTENSION_FUNCTION_LIST mxext_hdf5_extension_function_list;

MX_API mx_status_type mxext_hdf5_initialize( MX_EXTENSION *extension );

#endif /* __E_HDF5_H__ */

