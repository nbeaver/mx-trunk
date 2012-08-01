/*
 * Name:    i_radicon_taurus_rs232.c
 *
 * Purpose: MX driver to communicate with the serial port of
 *          a Radicon Taurus CMOS imaging detector.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2012 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_RADICON_TAURUS_RS232_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_ascii.h"
#include "mx_rs232.h"
#include "i_radicon_taurus_rs232.h"

MX_RECORD_FUNCTION_LIST mxi_radicon_taurus_rs232_record_function_list = {
	NULL,
	mxi_radicon_taurus_rs232_create_record_structures,
	mxi_radicon_taurus_rs232_finish_record_initialization,
	NULL,
	NULL,
	mxi_radicon_taurus_rs232_open
};

MX_RS232_FUNCTION_LIST mxi_radicon_taurus_rs232_rs232_function_list = {
	mxi_radicon_taurus_rs232_getchar,
	mxi_radicon_taurus_rs232_putchar,
	NULL,
	NULL,
	mxi_radicon_taurus_rs232_getline,
	mxi_radicon_taurus_rs232_putline,
	mxi_radicon_taurus_rs232_num_input_bytes_available,
	mxi_radicon_taurus_rs232_discard_unread_input
};

MX_RECORD_FIELD_DEFAULTS mxi_radicon_taurus_rs232_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_RS232_STANDARD_FIELDS,
	MXI_RADICON_TAURUS_RS232_STANDARD_FIELDS
};

long mxi_radicon_taurus_rs232_num_record_fields
		= sizeof( mxi_radicon_taurus_rs232_record_field_defaults )
		/ sizeof( mxi_radicon_taurus_rs232_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_radicon_taurus_rs232_rfield_def_ptr
			= &mxi_radicon_taurus_rs232_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxi_radicon_taurus_rs232_get_pointers( MX_RS232 *rs232,
			MX_RADICON_TAURUS_RS232 **radicon_taurus_rs232,
			const char *calling_fname )
{
	static const char fname[] = "mxi_radicon_taurus_rs232_get_pointers()";

	MX_RADICON_TAURUS_RS232 *radicon_taurus_rs232_ptr;
	MX_RECORD *xclib_record;

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RS232 pointer passed by '%s' was NULL.",
			calling_fname );
	}

	radicon_taurus_rs232_ptr = (MX_RADICON_TAURUS_RS232 *)
				rs232->record->record_type_struct;

	if ( radicon_taurus_rs232_ptr == (MX_RADICON_TAURUS_RS232 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RADICON_TAURUS_RS232 pointer for record '%s' is NULL.",
			rs232->record->name );
	}

	if ( radicon_taurus_rs232 != (MX_RADICON_TAURUS_RS232 **) NULL ) {
		*radicon_taurus_rs232 = radicon_taurus_rs232_ptr;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*==========================*/

MX_EXPORT mx_status_type
mxi_radicon_taurus_rs232_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxi_radicon_taurus_rs232_create_record_structures()";

	MX_RS232 *rs232;
	MX_RADICON_TAURUS_RS232 *radicon_taurus_rs232;

	/* Allocate memory for the necessary structures. */

	rs232 = (MX_RS232 *) malloc( sizeof(MX_RS232) );

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_RS232 structure." );
	}

	radicon_taurus_rs232 = (MX_RADICON_TAURUS_RS232 *)
				    malloc( sizeof(MX_RADICON_TAURUS_RS232) );

	if ( radicon_taurus_rs232 == (MX_RADICON_TAURUS_RS232 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	    "Cannot allocate memory for MX_RADICON_TAURUS_RS232 structure.");
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = rs232;
	record->record_type_struct = radicon_taurus_rs232;
	record->class_specific_function_list
				= &mxi_radicon_taurus_rs232_rs232_function_list;

	rs232->record = record;
	radicon_taurus_rs232->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_radicon_taurus_rs232_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxi_radicon_taurus_rs232_finish_record_initialization()";

	mx_status_type status;

#if MXI_RADICON_TAURUS_RS232_DEBUG
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

	/* Mark the radicon_taurus_rs232 device as being closed. */

	record->record_flags |= ( ~MXF_REC_OPEN );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_radicon_taurus_rs232_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_radicon_taurus_rs232_open()";

	MX_RS232 *rs232;
	MX_RADICON_TAURUS_RS232 *radicon_taurus_rs232;
	mx_status_type mx_status;

	radicon_taurus_rs232 = NULL;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL.");
	}

#if MXI_RADICON_TAURUS_RS232_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, record->name));
#endif

	rs232 = (MX_RS232 *) record->record_class_struct;

	mx_status = mxi_radicon_taurus_rs232_get_pointers( rs232,
						&radicon_taurus_rs232, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_convert_terminator_characters( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_radicon_taurus_rs232_getchar( MX_RS232 *rs232, char *c )
{
	static const char fname[] = "mxi_radicon_taurus_rs232_getchar()";

	MX_RADICON_TAURUS_RS232 *radicon_taurus_rs232;
	mx_status_type mx_status;

	mx_status = mxi_radicon_taurus_rs232_get_pointers( rs232,
						&radicon_taurus_rs232, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_getchar( radicon_taurus_rs232->serial_port_record,
					c, MXI_RADICON_TAURUS_RS232_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_RADICON_TAURUS_RS232_DEBUG
	MX_DEBUG(-2, ("%s: received 0x%x, '%c' from '%s'",
			fname, *c, *c, rs232->record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_radicon_taurus_rs232_putchar( MX_RS232 *rs232, char c )
{
	static const char fname[] = "mxi_radicon_taurus_rs232_putchar()";

	MX_RADICON_TAURUS_RS232 *radicon_taurus_rs232;
	mx_status_type mx_status;

	mx_status = mxi_radicon_taurus_rs232_get_pointers( rs232,
						&radicon_taurus_rs232, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_RADICON_TAURUS_RS232_DEBUG
	MX_DEBUG(-2, ("%s: sending 0x%x, '%c' to '%s'",
		fname, c, c, rs232->record->name));
#endif

	mx_status = mx_rs232_putchar( radicon_taurus_rs232->serial_port_record,
					c, MXI_RADICON_TAURUS_RS232_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_radicon_taurus_rs232_getline( MX_RS232 *rs232,
				char *buffer,
				size_t max_bytes_to_read,
				size_t *bytes_read )
{
	static const char fname[] = "mxi_radicon_taurus_rs232_getline()";

	MX_RADICON_TAURUS_RS232 *radicon_taurus_rs232;
	unsigned long num_bytes_available;
	char c;
	size_t bytes_to_delete;
	mx_status_type mx_status;

	mx_status = mxi_radicon_taurus_rs232_get_pointers( rs232,
						&radicon_taurus_rs232, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_getline( radicon_taurus_rs232->serial_port_record,
				buffer, max_bytes_to_read, bytes_read,
				MXI_RADICON_TAURUS_RS232_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if 0
	{
		size_t i;

		for ( i = 0; i < max_bytes_to_read; i++ ) {
			MX_DEBUG(-2,("%s: buffer[%lu] = '%c' (%#x)",
			fname, (unsigned long) i, buffer[i], buffer[i]));

			if ( buffer[i] == '\0' )
				break;
		}
	}
#endif
	/* If the first character read is the prompt character '>', then we
	 * must delete that byte from the buffer.  If the second character
	 * is a line feed (LF) character, then we must delete that too.
	 */

	if ( buffer[0] == '>' ) {
		bytes_to_delete = 1;

		if ( buffer[1] == MX_LF ) {
			bytes_to_delete++;
		}

		memmove( buffer, buffer + bytes_to_delete,
				max_bytes_to_read - bytes_to_delete );
	}

	/* Is there a single remaining byte in the input buffer?  If so,
	 * then this is probably a '>' character that is used as a prompt
	 * by the Radicon Taurus command interpreter.  In that case, we
	 * must read that single extra byte and throw it away.
	 */

	mx_status = mx_rs232_num_input_bytes_available(
				radicon_taurus_rs232->serial_port_record,
				&num_bytes_available );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( num_bytes_available == 1 ) {
		mx_status = mx_rs232_getchar( 
				radicon_taurus_rs232->serial_port_record,
				&c, MXI_RADICON_TAURUS_RS232_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( c != '>' ) {
			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"Serial port '%s' had a single input byte available "
			"that was not a '>' character.  Instead, it was "
			"a '%c' (%#x) character.",
				rs232->record->name, c, c );
		}
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_radicon_taurus_rs232_putline( MX_RS232 *rs232,
				char *buffer,
				size_t *bytes_written )
{
	static const char fname[] = "mxi_radicon_taurus_rs232_putline()";

	MX_RADICON_TAURUS_RS232 *radicon_taurus_rs232;
	mx_status_type mx_status;

	mx_status = mxi_radicon_taurus_rs232_get_pointers( rs232,
						&radicon_taurus_rs232, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_putline( radicon_taurus_rs232->serial_port_record,
				buffer, bytes_written,
				MXI_RADICON_TAURUS_RS232_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_radicon_taurus_rs232_num_input_bytes_available( MX_RS232 *rs232 )
{
	static const char fname[] =
		"mxi_radicon_taurus_rs232_num_input_bytes_available()";

	MX_RADICON_TAURUS_RS232 *radicon_taurus_rs232 = NULL;
	mx_status_type mx_status;

#if MXI_RADICON_TAURUS_RS232_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.",
			fname, rs232->record->name));
#endif

	mx_status = mxi_radicon_taurus_rs232_get_pointers( rs232,
						&radicon_taurus_rs232, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* How many bytes can be read without blocking? */

	mx_status = mx_rs232_num_input_bytes_available(
				radicon_taurus_rs232->serial_port_record,
				&(rs232->num_input_bytes_available) );

#if MXI_RADICON_TAURUS_RS232_DEBUG
	fprintf( stderr, "*%ld*", (long) rs232->num_input_bytes_available );
#endif
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_radicon_taurus_rs232_discard_unread_input( MX_RS232 *rs232 )
{
	static const char fname[] =
			"mxi_radicon_taurus_rs232_discard_unread_input()";

	MX_RADICON_TAURUS_RS232 *radicon_taurus_rs232 = NULL;
	mx_status_type mx_status;

	mx_status = mxi_radicon_taurus_rs232_get_pointers( rs232,
					&radicon_taurus_rs232, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_discard_unread_input(
				radicon_taurus_rs232->serial_port_record,
				MXI_RADICON_TAURUS_RS232_DEBUG );

	return mx_status;
}

