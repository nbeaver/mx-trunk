/*
 * Name:    i_camera_link_api.c
 *
 * Purpose: MX driver for generic Camera Link serial I/O interfaces.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_CAMERA_LINK_API_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_camera_link.h"
#include "i_camera_link_api.h"

MX_RECORD_FUNCTION_LIST mxi_camera_link_api_record_function_list = {
	NULL,
	mxi_camera_link_api_create_record_structures,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxi_camera_link_api_open,
	mxi_camera_link_api_close
};

MX_CAMERA_LINK_API_LIST mxi_camera_link_api_api_list = {
	NULL
};

MX_RECORD_FIELD_DEFAULTS mxi_camera_link_api_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_CAMERA_LINK_STANDARD_FIELDS,
	MXI_CAMERA_LINK_API_STANDARD_FIELDS
};

long mxi_camera_link_api_num_record_fields
		= sizeof( mxi_camera_link_api_record_field_defaults )
			/ sizeof( mxi_camera_link_api_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_camera_link_api_rfield_def_ptr
			= &mxi_camera_link_api_record_field_defaults[0];

/*-----*/

static mx_status_type
mxi_camera_link_api_get_pointers( MX_CAMERA_LINK *camera_link,
			MX_CAMERA_LINK_API **camera_link_api,
			MX_CAMERA_LINK_API_LIST **api_list_ptr,
			const char *calling_fname )
{
	static const char fname[] = "mxi_camera_link_api_get_pointers()";

	MX_RECORD *record;

	if ( camera_link == (MX_CAMERA_LINK *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_CAMERA_LINK pointer passed by '%s' was NULL.",
			calling_fname );
	}

	record = camera_link->record;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for the MX_CAMERA_LINK pointer "
		"passed was NULL." );
	}

	if ( camera_link_api != (MX_CAMERA_LINK_API **) NULL ) {
		*camera_link_api = record->record_type_struct;
	}

	if ( api_list_ptr != (MX_CAMERA_LINK_API_LIST **) NULL ) {
		*api_list_ptr = record->class_specific_function_list;
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ==== Public functions ==== */

MX_EXPORT mx_status_type
mxi_camera_link_api_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxi_camera_link_api_create_record_structures()";

	MX_CAMERA_LINK *camera_link;
	MX_CAMERA_LINK_API *camera_link_api;

	/* Allocate memory for the necessary structures. */

	camera_link = (MX_CAMERA_LINK *) malloc( sizeof(MX_CAMERA_LINK) );

	if ( camera_link == (MX_CAMERA_LINK *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for an MX_CAMERA_LINK structure." );
	}

	camera_link_api = (MX_CAMERA_LINK_API *)
				malloc( sizeof(MX_CAMERA_LINK_API) );

	if ( camera_link_api == (MX_CAMERA_LINK_API *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for an MX_CAMERA_LINK_API structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = camera_link;
	record->record_type_struct = camera_link_api;
	record->record_function_list =
				&mxi_camera_link_api_record_function_list;
	record->class_specific_function_list = &mxi_camera_link_api_api_list;

	camera_link->record = record;
	camera_link_api->record = record;
	camera_link_api->camera_link = camera_link;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_camera_link_api_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_camera_link_api_open()";

	MX_CAMERA_LINK *camera_link;
	MX_CAMERA_LINK_API *camera_link_api;
	MX_CAMERA_LINK_API_LIST *api_list;
	MX_DYNAMIC_LIBRARY *lib;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	camera_link = (MX_CAMERA_LINK *) record->record_class_struct;

	mx_status = mxi_camera_link_api_get_pointers( camera_link,
					&camera_link_api, &api_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_CAMERA_LINK_API_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s', library filename = '%s'",
		fname, record->name, camera_link_api->library_filename));
#endif

	/* Attempt to load the specified Camera Link dynamic library. */

	mx_status = mx_dynamic_library_open(
				camera_link_api->library_filename,
				&(camera_link_api->library) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Attempt to initialize all of the entries in
	 * the MX_CAMERA_LINK_API_LIST structure.
	 *
	 * It is not an error for some of the function
	 * pointers to be NULL.
	 */

	lib = camera_link_api->library;

	api_list->flush_port = (INT32 (*)(hSerRef))
			mx_dynamic_library_get_symbol_pointer(
					lib, "clFlushPort" );
					
	api_list->get_error_text =
		(INT32 (*)(const INT8 *, INT32, INT8 *, UINT32 *))
			mx_dynamic_library_get_symbol_pointer(
					lib, "clGetErrorText" );

	api_list->get_manufacturer_info =
		(INT32 (*)(INT8 *, UINT32 *, UINT32 *))
			mx_dynamic_library_get_symbol_pointer(
					lib, "clGetManufacturerInfo" );

	api_list->get_num_bytes_avail = (INT32 (*)(hSerRef, UINT32 *))
			mx_dynamic_library_get_symbol_pointer(
					lib, "clGetNumBytesAvail" );

	api_list->get_num_ports = (INT32 (*)(UINT32 *))
			mx_dynamic_library_get_symbol_pointer(
					lib, "clGetNumPorts" );

	api_list->get_num_serial_ports = (INT32 (*)(UINT32 *))
			mx_dynamic_library_get_symbol_pointer(
					lib, "clGetNumSerialPorts" );

	api_list->get_port_info =
	    (INT32 (*)(UINT32, INT8 *, UINT32 *, INT8 *, UINT32 *, UINT32 *))
			mx_dynamic_library_get_symbol_pointer(
					lib, "clGetPortInfo" );

	api_list->get_serial_port_identifier =
		(INT32 (*)(UINT32, INT8 *, UINT32 *))
			mx_dynamic_library_get_symbol_pointer(
					lib, "clGetSerialPortIdentifier" );

	api_list->get_supported_baud_rates = (INT32 (*)(hSerRef, UINT32 *))
			mx_dynamic_library_get_symbol_pointer(
					lib, "clGetSupportedBaudRates" );

	api_list->serial_close = (void (*)(hSerRef))
			mx_dynamic_library_get_symbol_pointer(
					lib, "clSerialClose" );

	api_list->serial_init = (INT32 (*)(UINT32, hSerRef *))
			mx_dynamic_library_get_symbol_pointer(
					lib, "clSerialInit" );

	api_list->serial_read = (INT32 (*)(hSerRef, INT8 *, UINT32 *, UINT32))
			mx_dynamic_library_get_symbol_pointer(
					lib, "clSerialRead" );

	api_list->serial_write = (INT32 (*)(hSerRef, INT8 *, UINT32 *, UINT32))
			mx_dynamic_library_get_symbol_pointer(
					lib, "clSerialWrite" );

	api_list->set_baud_rate = (INT32 (*)(hSerRef, UINT32))
			mx_dynamic_library_get_symbol_pointer(
					lib, "clSetBaudRate" );

	/* Initialize the Camera Link serial connection. */

	mx_status = mx_camera_link_open( record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_camera_link_api_close( MX_RECORD *record )
{
	static const char fname[] = "mxi_camera_link_api_close()";

	MX_CAMERA_LINK *camera_link;
	MX_CAMERA_LINK_API *camera_link_api;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	camera_link = (MX_CAMERA_LINK *) record->record_class_struct;

	mx_status = mxi_camera_link_api_get_pointers( camera_link,
					&camera_link_api, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Disconnect from the dynamic library. */

	mx_status = mx_dynamic_library_close( camera_link_api->library );

	return mx_status;
}

