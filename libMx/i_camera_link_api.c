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

#define MXI_CAMERA_LINK_API_DEBUG	TRUE

#include <stdio.h>

#define HAVE_CAMERA_LINK		TRUE

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

static void
mxi_camera_link_get_api_entry( MX_DYNAMIC_LIBRARY *library,
				const char *function_name,
				void **function_ptr )
{
	static const char fname[] = "mxi_camera_link_get_api_entry()";

	void *ptr;
	mx_status_type mx_status;

	mx_status = mx_dynamic_link_find_symbol( library,
						function_name, &ptr, TRUE );

	if ( mx_status.code != MXE_SUCCESS ) {
		*function_ptr = NULL;
	} else {
		*function_ptr = ptr;
	}

	MX_DEBUG(-2,
   ("%s: function_name = '%s', status code = %ld, ptr = %p, *function_ptr = %p",
		fname, function_name, mx_status.code, ptr, *function_ptr ));

	return;
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

	mx_status = mx_dynamic_link_open_library(
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

	mxi_camera_link_get_api_entry( lib, "clFlushPort",
				(void **) &(api_list->flush_port) );
					
	mxi_camera_link_get_api_entry( lib, "clGetErrorText",
				(void **) &(api_list->get_error_text) );

	mxi_camera_link_get_api_entry( lib, "clGetManufacturerInfo",
				(void **) &(api_list->get_manufacturer_info) );

	mxi_camera_link_get_api_entry( lib, "clGetNumBytesAvail",
				(void **) &(api_list->get_num_bytes_avail) );

	mxi_camera_link_get_api_entry( lib, "clGetNumPorts",
				(void **) &(api_list->get_num_ports) );

	mxi_camera_link_get_api_entry( lib, "clGetNumSerialPorts",
				(void **) &(api_list->get_num_serial_ports) );

	mxi_camera_link_get_api_entry( lib, "clGetPortInfo",
				(void **) &(api_list->get_port_info) );

	mxi_camera_link_get_api_entry( lib, "clGetSerialPortIdentifier",
			(void **) &(api_list->get_serial_port_identifier) );

	mxi_camera_link_get_api_entry( lib, "clGetSupportedBaudRates",
			(void **) &(api_list->get_supported_baud_rates) );

	mxi_camera_link_get_api_entry( lib, "clSerialClose",
				(void **) &(api_list->serial_close) );

	mxi_camera_link_get_api_entry( lib, "clSerialInit",
				(void **) &(api_list->serial_init) );

	mxi_camera_link_get_api_entry( lib, "clSerialRead",
				(void **) &(api_list->serial_read) );

	mxi_camera_link_get_api_entry( lib, "clSerialWrite",
				(void **) &(api_list->serial_write) );

	mxi_camera_link_get_api_entry( lib, "clSetBaudRate",
				(void **) &(api_list->set_baud_rate) );

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

	mx_status = mx_dynamic_link_close_library( camera_link_api->library );

	return mx_status;
}

