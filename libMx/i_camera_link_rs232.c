/*
 * Name:    i_camera_link_rs232.c
 *
 * Purpose: MX driver to treat a Camera Link serial interface
 *          as if it were an RS-232 port.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2006-2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_CAMERA_LINK_RS232_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_rs232.h"
#include "mx_camera_link.h"
#include "i_camera_link_rs232.h"

MX_RECORD_FUNCTION_LIST mxi_camera_link_rs232_record_function_list = {
	NULL,
	mxi_camera_link_rs232_create_record_structures,
	mxi_camera_link_rs232_finish_record_initialization,
	NULL,
	NULL,
	NULL,
	NULL,
	mxi_camera_link_rs232_open
};

MX_RS232_FUNCTION_LIST mxi_camera_link_rs232_rs232_function_list = {
	mxi_camera_link_rs232_getchar,
	mxi_camera_link_rs232_putchar,
	mxi_camera_link_rs232_read,
	mxi_camera_link_rs232_write,
	NULL,
	NULL,
	mxi_camera_link_rs232_num_input_bytes_available,
	mxi_camera_link_rs232_discard_unread_input
};

MX_RECORD_FIELD_DEFAULTS mxi_camera_link_rs232_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_RS232_STANDARD_FIELDS,
	MXI_CAMERA_LINK_RS232_STANDARD_FIELDS
};

long mxi_camera_link_rs232_num_record_fields
		= sizeof( mxi_camera_link_rs232_record_field_defaults )
			/ sizeof( mxi_camera_link_rs232_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_camera_link_rs232_rfield_def_ptr
			= &mxi_camera_link_rs232_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxi_camera_link_rs232_get_pointers( MX_RS232 *rs232,
			MX_CAMERA_LINK_RS232 **camera_link_rs232,
			MX_RECORD **camera_link_record,
			const char *calling_fname )
{
	static const char fname[] = "mxi_camera_link_rs232_get_pointers()";

	MX_CAMERA_LINK_RS232 *camera_link_rs232_ptr;

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RS232 pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( rs232->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_RECORD pointer for the "
			"MX_RS232 pointer passed by '%s' is NULL.",
			calling_fname );
	}

	camera_link_rs232_ptr = (MX_CAMERA_LINK_RS232 *)
				rs232->record->record_type_struct;

	if ( camera_link_rs232_ptr == (MX_CAMERA_LINK_RS232 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_CAMERA_LINK_RS232 pointer for record '%s' is NULL.",
			rs232->record->name );
	}

	if ( camera_link_rs232 != (MX_CAMERA_LINK_RS232 **) NULL ) {
		*camera_link_rs232 = camera_link_rs232_ptr;
	}

	if ( camera_link_record != (MX_RECORD **) NULL ) {
		*camera_link_record = camera_link_rs232_ptr->camera_link_record;

		if ( (*camera_link_record) == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The camera_link_record pointer for record '%s' is NULL.",
				rs232->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*==========================*/

MX_EXPORT mx_status_type
mxi_camera_link_rs232_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxi_camera_link_rs232_create_record_structures()";

	MX_RS232 *rs232;
	MX_CAMERA_LINK_RS232 *camera_link_rs232;

	/* Allocate memory for the necessary structures. */

	rs232 = (MX_RS232 *) malloc( sizeof(MX_RS232) );

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_RS232 structure." );
	}

	camera_link_rs232 =
		(MX_CAMERA_LINK_RS232 *) malloc( sizeof(MX_CAMERA_LINK_RS232) );

	if ( camera_link_rs232 == (MX_CAMERA_LINK_RS232 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_CAMERA_LINK_RS232 structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = rs232;
	record->record_type_struct = camera_link_rs232;
	record->class_specific_function_list
				= &mxi_camera_link_rs232_rs232_function_list;

	rs232->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_camera_link_rs232_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxi_camera_link_rs232_finish_record_initialization()";

	mx_status_type status;

#if MXI_CAMERA_LINK_RS232_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL.");
	}

	/* Check to see if the RS-232 parameters are valid. */

	status = mx_rs232_check_port_parameters( record );

	if ( status.code != MXE_SUCCESS )
		return status;

	/* Mark the camera_link_rs232 device as being closed. */

	record->record_flags |= ( ~MXF_REC_OPEN );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_camera_link_rs232_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_camera_link_rs232_open()";

	MX_RS232 *rs232;
	MX_CAMERA_LINK_RS232 *camera_link_rs232;
	MX_RECORD *camera_link_record;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL.");
	}

#if MXI_CAMERA_LINK_RS232_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, record->name));
#endif

	rs232 = (MX_RS232 *) record->record_class_struct;

	mx_status = mxi_camera_link_rs232_get_pointers( rs232,
				&camera_link_rs232, &camera_link_record, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_convert_terminator_characters( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_camera_link_rs232_getchar( MX_RS232 *rs232, char *c )
{
	static const char fname[] = "mxi_camera_link_rs232_getchar()";

	MX_RECORD *camera_link_record;
	size_t buffer_size;
	mx_status_type mx_status;

	mx_status = mxi_camera_link_rs232_get_pointers( rs232,
					NULL, &camera_link_record, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( c == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The character pointer 'c' passed is NULL." );
	}

	buffer_size = 1;

	mx_status = mx_camera_link_serial_read( camera_link_record,
						c, &buffer_size, -1 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_CAMERA_LINK_RS232_DEBUG
	MX_DEBUG(-2, ("%s: received 0x%x, '%c' from '%s'",
			fname, *c, *c, rs232->record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_camera_link_rs232_putchar( MX_RS232 *rs232, char c )
{
	static const char fname[] = "mxi_camera_link_rs232_putchar()";

	MX_RECORD *camera_link_record;
	size_t buffer_size;
	mx_status_type mx_status;

#if MXI_CAMERA_LINK_RS232_DEBUG
	MX_DEBUG(-2, ("%s: sending 0x%x, '%c' to '%s'",
		fname, c, c, rs232->record->name));
#endif

	mx_status = mxi_camera_link_rs232_get_pointers( rs232,
					NULL, &camera_link_record, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	buffer_size = 1;

	mx_status = mx_camera_link_serial_write( camera_link_record,
						&c, &buffer_size, -1 );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_camera_link_rs232_read( MX_RS232 *rs232,
				char *buffer,
				size_t max_bytes_to_read,
				size_t *bytes_read )
{
	static const char fname[] = "mxi_camera_link_rs232_read()";

	MX_RECORD *camera_link_record;
	size_t cl_buffer_size;
	mx_status_type mx_status;

	mx_status = mxi_camera_link_rs232_get_pointers( rs232,
					NULL, &camera_link_record, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( buffer == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The buffer pointer passed for record '%s' is NULL.",
			rs232->record->name );
	}

#if MXI_CAMERA_LINK_RS232_DEBUG
	MX_DEBUG(-2,("%s: max_bytes_to_read = %lu",
		fname, (unsigned long) max_bytes_to_read));
#endif

	cl_buffer_size = max_bytes_to_read;

	mx_status = mx_camera_link_serial_read( camera_link_record,
						buffer, &cl_buffer_size, -1 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_CAMERA_LINK_RS232_DEBUG
	{
		int i;

		MX_DEBUG(-2,("%s: received buffer from '%s'",
			fname, rs232->record->name ));

		for ( i = 0; i < cl_buffer_size; i++ ) {
			MX_DEBUG(-2,("%s: buffer[%d] = %#x '%c'",
				fname, i, buffer[i], buffer[i]));
		}
	}
#endif

	if ( bytes_read != NULL ) {
		*bytes_read = cl_buffer_size;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_camera_link_rs232_write( MX_RS232 *rs232,
				char *buffer,
				size_t max_bytes_to_write,
				size_t *bytes_written )
{
	static const char fname[] = "mxi_camera_link_rs232_write()";

	MX_RECORD *camera_link_record;
	size_t cl_buffer_size;
	mx_status_type mx_status;

	mx_status = mxi_camera_link_rs232_get_pointers( rs232,
					NULL, &camera_link_record, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( buffer == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The buffer pointer passed for record '%s' is NULL.",
			rs232->record->name );
	}

#if MXI_CAMERA_LINK_RS232_DEBUG
	{
		int i;

		MX_DEBUG(-2,("%s: sending buffer to '%s'",
			fname, rs232->record->name ));

		for ( i = 0; i < max_bytes_to_write; i++ ) {
			MX_DEBUG(-2,("%s: buffer[%d] = %#x '%c'",
				fname, i, buffer[i], buffer[i]));
		}
	}
#endif

	cl_buffer_size = max_bytes_to_write;

	mx_status = mx_camera_link_serial_write( camera_link_record,
						buffer, &cl_buffer_size, -1 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( bytes_written != NULL ) {
		*bytes_written = cl_buffer_size;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_camera_link_rs232_num_input_bytes_available( MX_RS232 *rs232 )
{
	static const char fname[] =
		"mxi_camera_link_rs232_num_input_bytes_available()";

	MX_RECORD *camera_link_record;
	size_t num_bytes_avail;
	mx_status_type mx_status;

	mx_status = mxi_camera_link_rs232_get_pointers( rs232,
					NULL, &camera_link_record, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_camera_link_get_num_bytes_avail( camera_link_record,
							&num_bytes_avail );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	rs232->num_input_bytes_available = num_bytes_avail;

#if MXI_CAMERA_LINK_RS232_DEBUG
	fprintf( stderr, "*%ld*", (long) rs232->num_input_bytes_available );
#endif
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_camera_link_rs232_discard_unread_input( MX_RS232 *rs232 )
{
	static const char fname[] =
			"mxi_camera_link_rs232_discard_unread_input()";

	MX_RECORD *camera_link_record;
	mx_status_type mx_status;

	mx_status = mxi_camera_link_rs232_get_pointers( rs232,
					NULL, &camera_link_record, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_camera_link_flush_port( camera_link_record );

	return mx_status;
}

