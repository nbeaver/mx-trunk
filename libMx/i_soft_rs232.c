/*
 * Name:    i_soft_rs232.c
 *
 * Purpose: MX driver for software emulated RS-232 devices.
 *
 *          The driver works by copying input strings to a local buffer
 *          and then sending them back when asked.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2001, 2003, 2005, 2008, 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_rs232.h"
#include "i_soft_rs232.h"

/* Do we show chars using MX_DEBUG() in the getchar and putchar functions? */

#define MXI_SOFT_RS232_DEBUG			FALSE

MX_RECORD_FUNCTION_LIST mxi_soft_rs232_record_function_list = {
	NULL,
	mxi_soft_rs232_create_record_structures,
	mxi_soft_rs232_finish_record_initialization,
	NULL,
	NULL,
	mxi_soft_rs232_open
};

MX_RS232_FUNCTION_LIST mxi_soft_rs232_rs232_function_list = {
	mxi_soft_rs232_getchar,
	mxi_soft_rs232_putchar,
	NULL,
	NULL,
	NULL,
	NULL,
	mxi_soft_rs232_num_input_bytes_available,
	mxi_soft_rs232_discard_unread_input,
	mxi_soft_rs232_discard_unwritten_output
};

MX_RECORD_FIELD_DEFAULTS mxi_soft_rs232_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_RS232_STANDARD_FIELDS
};

long mxi_soft_rs232_num_record_fields
		= sizeof( mxi_soft_rs232_record_field_defaults )
			/ sizeof( mxi_soft_rs232_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_soft_rs232_rfield_def_ptr
			= &mxi_soft_rs232_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxi_soft_rs232_get_pointers( MX_RS232 *rs232,
			MX_SOFT_RS232 **soft_rs232,
			const char *calling_fname )
{
	static const char fname[] = "mxi_soft_rs232_get_pointers()";

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RS232 pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( soft_rs232 == (MX_SOFT_RS232 **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SOFT_RS232 pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( rs232->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_RECORD pointer for the "
			"MX_RS232 pointer passed by '%s' is NULL.",
			calling_fname );
	}

	*soft_rs232 = (MX_SOFT_RS232 *) rs232->record->record_type_struct;

	if ( *soft_rs232 == (MX_SOFT_RS232 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_SOFT_RS232 pointer for record '%s' is NULL.",
			rs232->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*==========================*/

MX_EXPORT mx_status_type
mxi_soft_rs232_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_soft_rs232_create_record_structures()";

	MX_RS232 *rs232;
	MX_SOFT_RS232 *soft_rs232 = NULL;

	/* Allocate memory for the necessary structures. */

	rs232 = (MX_RS232 *) malloc( sizeof(MX_RS232) );

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_RS232 structure." );
	}

	soft_rs232 = (MX_SOFT_RS232 *) malloc( sizeof(MX_SOFT_RS232) );

	if ( soft_rs232 == (MX_SOFT_RS232 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SOFT_RS232 structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = rs232;
	record->record_type_struct = soft_rs232;
	record->class_specific_function_list
				= &mxi_soft_rs232_rs232_function_list;

	rs232->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_soft_rs232_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxi_soft_rs232_finish_record_initialization()";

	MX_SOFT_RS232 *soft_rs232 = NULL;
	mx_status_type status;

	MX_DEBUG( 2,("%s invoked.", fname));

	soft_rs232 = (MX_SOFT_RS232 *) (record->record_type_struct);

	if ( soft_rs232 == (MX_SOFT_RS232 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_SOFT_RS232 structure pointer for '%s' is NULL.",
			record->name );
	}

	/* Check to see if the RS-232 parameters are valid. */

	status = mx_rs232_check_port_parameters( record );

	if ( status.code != MXE_SUCCESS )
		return status;

	/* Mark the soft_rs232 device as being closed. */

	record->record_flags |= ( ~MXF_REC_OPEN );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_soft_rs232_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_soft_rs232_open()";

	MX_RS232 *rs232;
	MX_SOFT_RS232 *soft_rs232 = NULL;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL.");
	}

	rs232 = (MX_RS232 *) record->record_class_struct;

	mx_status = mxi_soft_rs232_get_pointers( rs232, &soft_rs232, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	strcpy( soft_rs232->buffer, "" );

	soft_rs232->read_ptr  = soft_rs232->buffer;
	soft_rs232->write_ptr = soft_rs232->buffer;

	soft_rs232->end_ptr = soft_rs232->buffer + MXI_SOFT_RS232_BUFFER_LENGTH;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_soft_rs232_getchar( MX_RS232 *rs232, char *c )
{
	static const char fname[] = "mxi_soft_rs232_getchar()";

	MX_SOFT_RS232 *soft_rs232 = NULL;
	mx_status_type mx_status;

	mx_status = mxi_soft_rs232_get_pointers( rs232, &soft_rs232, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( c == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The character pointer 'c' passed is NULL." );
	}

	if ( soft_rs232->read_ptr >= soft_rs232->write_ptr ) {

		/* If no characters are available to be read,
		 * return a null byte.
		 */

		*c = '\0';

	} else {
		/* Otherwise, return the character at the current position
		 * of the read pointer and increment the read pointer.
		 */

		*c = *(soft_rs232->read_ptr);

		(soft_rs232->read_ptr)++;

		/* If we have gone past the end of the buffer, loop back
		 * to the beginning.
		 */

		if ( soft_rs232->read_ptr >= soft_rs232->end_ptr ) {
			soft_rs232->read_ptr = soft_rs232->buffer;
		}
	}

#if MXI_SOFT_RS232_DEBUG
	MX_DEBUG(-2, ("%s: received 0x%x, '%c'", fname, *c, *c));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_soft_rs232_putchar( MX_RS232 *rs232, char c )
{
	static const char fname[] = "mxi_soft_rs232_putchar()";

	MX_SOFT_RS232 *soft_rs232 = NULL;
	mx_status_type mx_status;

#if MXI_SOFT_RS232_DEBUG
	MX_DEBUG(-2, ("%s: sending 0x%x, '%c'", fname, c, c));
#endif

	mx_status = mxi_soft_rs232_get_pointers( rs232, &soft_rs232, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( soft_rs232->write_ptr > soft_rs232->end_ptr ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The character buffer for soft RS-232 port '%s' is full.",
			rs232->record->name );
	}

	/* Save the character sent to us and move to the next buffer position.*/

	*(soft_rs232->write_ptr) = c;

	(soft_rs232->write_ptr)++;

	/* If we have gone past the end of the buffer, loop back
	 * to the beginning.
	 */

	if ( soft_rs232->write_ptr >= soft_rs232->end_ptr ) {
		soft_rs232->write_ptr = soft_rs232->buffer;
	}

	/* Zero out the character here so that the buffer is null terminated.
	 */

	*(soft_rs232->write_ptr) = '\0';

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_soft_rs232_num_input_bytes_available( MX_RS232 *rs232 )
{
	static const char fname[] =
		"mxi_soft_rs232_num_input_bytes_available()";

	MX_SOFT_RS232 *soft_rs232 = NULL;
	mx_status_type mx_status;

	mx_status = mxi_soft_rs232_get_pointers( rs232, &soft_rs232, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( soft_rs232->read_ptr == NULL ) {
		rs232->num_input_bytes_available = 0;
	} else {
		rs232->num_input_bytes_available = 1;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_soft_rs232_discard_unread_input( MX_RS232 *rs232 )
{
	static const char fname[] = "mxi_soft_rs232_discard_unread_input()";

	MX_SOFT_RS232 *soft_rs232 = NULL;
	mx_status_type mx_status;

	mx_status = mxi_soft_rs232_get_pointers( rs232, &soft_rs232, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	soft_rs232->read_ptr = NULL;
	soft_rs232->write_ptr = soft_rs232->buffer;

	*(soft_rs232->write_ptr) = '\0';

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_soft_rs232_discard_unwritten_output( MX_RS232 *rs232 )
{
	static const char fname[] = "mxi_soft_rs232_discard_unwritten_output()";

	MX_SOFT_RS232 *soft_rs232 = NULL;
	mx_status_type mx_status;

	mx_status = mxi_soft_rs232_get_pointers( rs232, &soft_rs232, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	soft_rs232->read_ptr = NULL;
	soft_rs232->write_ptr = soft_rs232->buffer;

	*(soft_rs232->write_ptr) = '\0';

	return MX_SUCCESSFUL_RESULT;
}

