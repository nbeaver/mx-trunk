/*
 * Name:     mx_module.h
 *
 * Purpose:  Supports dynamically loadable MX modules.
 *
 * Author:   William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2010-2011 Illinois Institute of Technology
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

typedef struct {
	char name[MXU_EXTENSION_NAME_LENGTH+1];
	unsigned long mx_version;
} MX_EXTENSION;

typedef struct mx_module_type {
	char name[MXU_MODULE_NAME_LENGTH+1];
	unsigned long mx_version;

	MX_DRIVER *driver_table;
	MX_EXTENSION *extension_table;

	MX_DYNAMIC_LIBRARY *library;
} MX_MODULE;

MX_API mx_status_type mx_load_module( char *filename,
					MX_RECORD *record_list,
					MX_MODULE **module );

MX_API mx_status_type mx_get_module( char *module_name,
					MX_RECORD *record_list,
					MX_MODULE **module );

#ifdef __cplusplus
}
#endif

#endif /* __MX_MODULE_H__ */

