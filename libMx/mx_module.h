/*
 * Name:     mx_module.h
 *
 * Purpose:  Supports dynamically loadable MX modules.
 *
 * Author:   William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2010-2012, 2014-2015, 2018 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_MODULE_H__
#define __MX_MODULE_H__

#include "mx_dynamic_library.h"

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

#define MXU_MODULE_NAME_LENGTH		40
#define MXU_EXTENSION_NAME_LENGTH	40

#define MXF_EXT_IS_DISABLED		0x1
#define MXF_EXT_HAS_SCRIPT_LANGUAGE	0x2

typedef struct {
	char name[MXU_EXTENSION_NAME_LENGTH+1];
	struct mx_extension_function_list_type *extension_function_list;
	struct mx_module_type *module;
	MX_RECORD *record_list;
	unsigned long extension_flags;
	void *ext_private;
} MX_EXTENSION;

typedef struct mx_extension_function_list_type {
	mx_status_type ( *initialize )( MX_EXTENSION * );
	mx_status_type ( *finalize )( MX_EXTENSION * );
	mx_status_type ( *call )( MX_EXTENSION *, int request_code,
						int argc, void **argv );
	mx_status_type ( *call_string )( MX_EXTENSION*, char *, char *, size_t);
} MX_EXTENSION_FUNCTION_LIST;

typedef struct mx_module_type {
	char name[MXU_MODULE_NAME_LENGTH+1];
	unsigned long mx_version;

	MX_DRIVER *driver_table;
	MX_EXTENSION *extension_table;

	MX_DYNAMIC_LIBRARY *library;
	MX_RECORD *record_list;
} MX_MODULE;

typedef mx_bool_type (*MX_MODULE_INIT)( MX_MODULE * );

MX_API mx_status_type mx_load_module( char *filename,
					MX_RECORD *record_list,
					MX_MODULE **module );

MX_API mx_status_type mx_get_module( char *module_name,
					MX_RECORD *record_list,
					MX_MODULE **module );

MX_API mx_status_type mx_get_extension_from_module( MX_MODULE *module,
					char *extension_name,
					MX_EXTENSION **extension );

MX_API mx_status_type mx_get_extension( char *extension_name,
					MX_RECORD *record_list,
					MX_EXTENSION **extension );

MX_API mx_status_type mx_finalize_extensions( MX_RECORD *record_list );

MX_API MX_EXTENSION *mx_get_default_script_extension( void );

MX_API void mx_set_default_script_extension( MX_EXTENSION *extension );

MX_API mx_status_type mx_extension_call( MX_EXTENSION *extension,
						int request_code,
						int argc, void **argv );

MX_API mx_status_type mx_extension_call_string( MX_EXTENSION *extension,
						char *string_arguments,
						char *string_response,
						size_t max_response_length );

MX_API mx_status_type mx_extension_call_by_name( char *extension_name,
						MX_RECORD *record_list,
						int request_code,
						int argc, void **argv );

MX_API mx_status_type mx_extension_call_string_by_name( char *extension_name,
						MX_RECORD *record_list,
						char *string_arguments,
						char *string_response,
						size_t max_response_length );

#ifdef __cplusplus
}
#endif

#endif /* __MX_MODULE_H__ */

