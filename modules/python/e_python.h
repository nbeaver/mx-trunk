/*
 * Name:    e_python.h
 *
 * Purpose: Header for the MX python extensoin.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2014 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __E_PYTHON_H__
#define __E_PYTHON_H__

/*----*/

extern MX_EXTENSION_FUNCTION_LIST mxext_python_extension_function_list;

MX_API mx_status_type mxext_python_init( MX_EXTENSION *extension );

MX_API mx_status_type mxext_python_call( MX_EXTENSION *extension,
					int argc,
					void **argv );

#endif /* __E_PYTHON_H__ */

