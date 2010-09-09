/*
 * Name:    i_soft_camera_link.c
 *
 * Purpose: MX driver for software-emulated Camera Link serial I/O interfaces.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2007, 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_SOFT_CAMERA_LINK_DEBUG	FALSE

#ifndef IS_MX_DRIVER
#define IS_MX_DRIVER  /* FIXME: It should not be necessary to have this here. */
#endif

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_camera_link.h"
#include "i_soft_camera_link.h"

MX_RECORD_FUNCTION_LIST mxi_soft_camera_link_record_function_list = {
	NULL,
	mxi_soft_camera_link_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxi_soft_camera_link_open,
	mxi_soft_camera_link_close
};

MX_CAMERA_LINK_API_LIST mxi_soft_camera_link_api_list = {
	NULL,
	NULL,
	NULL,
	mxi_soft_camera_link_get_num_bytes_avail,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxi_soft_camera_link_serial_close,
	mxi_soft_camera_link_serial_init,
	mxi_soft_camera_link_serial_read,
	mxi_soft_camera_link_serial_write,
	mxi_soft_camera_link_set_baud_rate,
	mxi_soft_camera_link_set_cc_line
};

MX_RECORD_FIELD_DEFAULTS mxi_soft_camera_link_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_CAMERA_LINK_STANDARD_FIELDS,
	MXI_SOFT_CAMERA_LINK_STANDARD_FIELDS
};

long mxi_soft_camera_link_num_record_fields
		= sizeof( mxi_soft_camera_link_record_field_defaults )
			/ sizeof( mxi_soft_camera_link_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_soft_camera_link_rfield_def_ptr
			= &mxi_soft_camera_link_record_field_defaults[0];

/*-----*/

static MX_SOFT_CAMERA_LINK_PORT *mxi_soft_camera_link_port = NULL;

/*-----*/

static mx_status_type
mxi_soft_camera_link_get_pointers( MX_CAMERA_LINK *camera_link,
			MX_SOFT_CAMERA_LINK **soft_camera_link,
			MX_CAMERA_LINK_API_LIST **api_list_ptr,
			const char *calling_fname )
{
	static const char fname[] = "mxi_soft_camera_link_get_pointers()";

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

	if ( soft_camera_link != (MX_SOFT_CAMERA_LINK **) NULL ) {
		*soft_camera_link = record->record_type_struct;
	}

	if ( api_list_ptr != (MX_CAMERA_LINK_API_LIST **) NULL ) {
		*api_list_ptr = record->class_specific_function_list;
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ==== Public functions ==== */

MX_EXPORT mx_status_type
mxi_soft_camera_link_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxi_soft_camera_link_create_record_structures()";

	MX_CAMERA_LINK *camera_link;
	MX_SOFT_CAMERA_LINK *soft_camera_link;

	/* Allocate memory for the necessary structures. */

	camera_link = (MX_CAMERA_LINK *) malloc( sizeof(MX_CAMERA_LINK) );

	if ( camera_link == (MX_CAMERA_LINK *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for an MX_CAMERA_LINK structure." );
	}

	soft_camera_link = (MX_SOFT_CAMERA_LINK *)
				malloc( sizeof(MX_SOFT_CAMERA_LINK) );

	if ( soft_camera_link == (MX_SOFT_CAMERA_LINK *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for an MX_SOFT_CAMERA_LINK structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = camera_link;
	record->record_type_struct = soft_camera_link;
	record->record_function_list =
				&mxi_soft_camera_link_record_function_list;
	record->class_specific_function_list = &mxi_soft_camera_link_api_list;

	camera_link->record = record;
	soft_camera_link->record = record;
	soft_camera_link->camera_link = camera_link;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_soft_camera_link_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_soft_camera_link_open()";

	MX_CAMERA_LINK *camera_link;
	MX_SOFT_CAMERA_LINK *soft_camera_link;
	MX_SOFT_CAMERA_LINK_PORT *port;
	MX_CAMERA_LINK_API_LIST *api_list;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	camera_link = (MX_CAMERA_LINK *) record->record_class_struct;

	/* Suppress GCC 4 uninitialized variable warning. */

	soft_camera_link = NULL;

	mx_status = mxi_soft_camera_link_get_pointers( camera_link,
					&soft_camera_link, &api_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_SOFT_CAMERA_LINK_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'", fname, record->name ));
#endif

	if ( mxi_soft_camera_link_port != NULL ) {
		return mx_error( MXE_NOT_AVAILABLE, fname,
		"Only one 'soft_camera_link' record may be created "
		"for a given process." );
	}

	/* Allocate memory for the MX_SOFT_CAMERA_LINK_PORT structure. */

	port = malloc( sizeof(MX_SOFT_CAMERA_LINK_PORT) );

	if ( port == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to allocate an "
			"MX_SOFT_CAMERA_LINK_PORT structure." );
	}

	port->record = record;
	port->camera_link = camera_link;
	port->soft_camera_link = soft_camera_link;

	mxi_soft_camera_link_port = port;

	mx_status = mx_camera_link_open( record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_soft_camera_link_close( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT INT32 MX_CLCALL
mxi_soft_camera_link_get_num_bytes_avail( hSerRef serial_ref,
					UINT32 *num_bytes )
{
#if MXI_SOFT_CAMERA_LINK_DEBUG
	static const char fname[] =
		"mxi_soft_camera_link_get_num_bytes_avail()";
#endif

	MX_SOFT_CAMERA_LINK_PORT *port;

	if ( serial_ref == NULL ) {
		return CL_ERR_INVALID_REFERENCE;
	}

	port = serial_ref;

#if MXI_SOFT_CAMERA_LINK_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, port->record->name));
#endif

	/* How many bytes can be read without blocking? */

	*num_bytes = 1;

	return CL_ERR_NO_ERR;
}

MX_EXPORT void MX_CLCALL
mxi_soft_camera_link_serial_close( hSerRef serial_ref )
{
#if MXI_SOFT_CAMERA_LINK_DEBUG
	static const char fname[] = "mxi_soft_camera_link_serial_close()";
#endif

	MX_SOFT_CAMERA_LINK_PORT *port;

	if ( serial_ref == NULL ) {

#if MXI_SOFT_CAMERA_LINK_DEBUG
		MX_DEBUG(-2,("%s invoked for a NULL pointer.", fname));
#endif
		return;
	}

	port = serial_ref;

#if MXI_SOFT_CAMERA_LINK_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, port->record->name));
#endif

	return;
}

MX_EXPORT INT32 MX_CLCALL
mxi_soft_camera_link_serial_init( UINT32 serial_index, hSerRef *serial_ref_ptr )
{
#if MXI_SOFT_CAMERA_LINK_DEBUG
	static const char fname[] = "mxi_soft_camera_link_serial_init()";

	MX_DEBUG(-2,("%s invoked for serial index %lu.",
		fname, (unsigned long) serial_index ));
#endif

	if ( mxi_soft_camera_link_port == NULL ) {
		return CL_ERR_ERROR_NOT_FOUND;
	}

	*serial_ref_ptr = mxi_soft_camera_link_port;

	return CL_ERR_NO_ERR;
}

MX_EXPORT INT32 MX_CLCALL
mxi_soft_camera_link_serial_read( hSerRef serial_ref, INT8 *buffer,
				UINT32 *num_bytes, UINT32 serial_timeout )
{
	static const char fname[] = "mxi_soft_camera_link_serial_read()";

	MX_SOFT_CAMERA_LINK_PORT *port;

	if ( serial_ref == NULL ) {
		return CL_ERR_INVALID_REFERENCE;
	}

	port = serial_ref;

#if MXI_SOFT_CAMERA_LINK_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, port->record->name));
#endif

	if ( buffer == (INT8 *) NULL ) {
		mx_error( MXE_NULL_ARGUMENT, fname,
		"The buffer pointer passed was NULL." );

		return CL_ERR_BUFFER_TOO_SMALL;
	}
	if ( num_bytes == (UINT32 *) NULL ) {
		mx_error( MXE_NULL_ARGUMENT, fname,
		"The num_bytes pointer passed was NULL." );

		return CL_ERR_BUFFER_TOO_SMALL;
	}

	if ( *num_bytes > 0 ) {
		*buffer = '\0';
	}

	return CL_ERR_NO_ERR;
}

MX_EXPORT INT32 MX_CLCALL
mxi_soft_camera_link_serial_write( hSerRef serial_ref, INT8 *buffer,
				UINT32 *num_bytes, UINT32 serial_timeout )
{
#if MXI_SOFT_CAMERA_LINK_DEBUG
	static const char fname[] = "mxi_soft_camera_link_serial_write()";
#endif

	MX_SOFT_CAMERA_LINK_PORT *port;

	if ( serial_ref == NULL ) {
		return CL_ERR_INVALID_REFERENCE;
	}

	port = serial_ref;

#if MXI_SOFT_CAMERA_LINK_DEBUG
	MX_DEBUG(-2,
		("%s invoked for record '%s'.", fname, port->record->name));

	{
		int i;

		for ( i = 0; i < *num_bytes; i++ ) {
			MX_DEBUG(-2,("%s: buffer[%d] = %#x '%c'",
				fname, i, buffer[i], buffer[i]));
		}
	}
#endif

	return CL_ERR_NO_ERR;
}

MX_EXPORT INT32 MX_CLCALL
mxi_soft_camera_link_set_baud_rate( hSerRef serial_ref, UINT32 baud_rate )
{
#if MXI_SOFT_CAMERA_LINK_DEBUG
	static const char fname[] = "mxi_soft_camera_link_set_baud_rate()";
#endif

	MX_SOFT_CAMERA_LINK_PORT *port;

	if ( serial_ref == NULL ) {
		return CL_ERR_INVALID_REFERENCE;
	}

	port = serial_ref;

#if MXI_SOFT_CAMERA_LINK_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, port->record->name));

	MX_DEBUG(-40,("%s: baud_rate = %lu", fname, (unsigned long) baud_rate));
#endif

	return CL_ERR_NO_ERR;
}

MX_EXPORT INT32 MX_CLCALL
mxi_soft_camera_link_set_cc_line( hSerRef serial_ref,
					UINT32 cc_line_number,
					UINT32 cc_line_state )
{
#if MXI_SOFT_CAMERA_LINK_DEBUG
	static const char fname[] = "mxi_soft_camera_link_set_cc_line()";
#endif

	MX_SOFT_CAMERA_LINK_PORT *port;

	if ( serial_ref == NULL ) {
		return CL_ERR_INVALID_REFERENCE;
	}

	port = serial_ref;

#if MXI_SOFT_CAMERA_LINK_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, port->record->name));
#endif

#if MXI_SOFT_CAMERA_LINK_DEBUG
	MX_DEBUG(-2,("%s: cc_line_number = %d, cc_line_state = %d",
		fname, (int) cc_line_number, (int) cc_line_state));
#endif

	return CL_ERR_NO_ERR;
}

