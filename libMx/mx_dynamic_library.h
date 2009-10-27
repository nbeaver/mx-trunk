/*
 * Name:    mx_dynamic_library.h
 *
 * Purpose: This header defines functions for loading dynamic libraries
 *          at run time and for searching for symbols in them.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------
 *
 * Copyright 2007, 2009 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_DYNAMIC_LIBRARY_H__
#define __MX_DYNAMIC_LIBRARY_H__

#include "mx_stdint.h"

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	void *object;
	char filename[MXU_FILENAME_LENGTH+1];
} MX_DYNAMIC_LIBRARY;

MX_API mx_status_type mx_dynamic_library_open( const char *filename,
						MX_DYNAMIC_LIBRARY **library );

MX_API mx_status_type mx_dynamic_library_close( MX_DYNAMIC_LIBRARY *library );

MX_API mx_status_type mx_dynamic_library_find_symbol(
						MX_DYNAMIC_LIBRARY *library,
						const char *symbol_name,
						void **symbol_pointer,
						mx_bool_type quiet_flag );

MX_API void *mx_dynamic_library_get_symbol_pointer( MX_DYNAMIC_LIBRARY *library,
						const char *symbol_name );

MX_API mx_status_type mx_dynamic_library_get_function_name_from_address(
						void *address,
						char *function_name,
						size_t max_name_length );

#ifdef __cplusplus
}
#endif

#endif /* __MX_DYNAMIC_LIBRARY_H_ */

