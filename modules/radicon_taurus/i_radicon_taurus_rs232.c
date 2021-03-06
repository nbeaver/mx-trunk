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
#include "mx_image.h"
#include "mx_area_detector.h"
#include "mx_rs232.h"
#include "d_radicon_taurus.h"
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
			MX_RADICON_TAURUS **radicon_taurus,
			MX_RECORD **serial_port_record,
			const char *calling_fname )
{
	static const char fname[] = "mxi_radicon_taurus_rs232_get_pointers()";

	MX_RADICON_TAURUS_RS232 *radicon_taurus_rs232_ptr = NULL;
	MX_RECORD *radicon_taurus_record = NULL;
	MX_RADICON_TAURUS *radicon_taurus_ptr = NULL;

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

	radicon_taurus_record = radicon_taurus_rs232_ptr->radicon_taurus_record;

	if ( radicon_taurus_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The radicon_taurus_record pointer for "
		"RS232 record '%s' is NULL.", rs232->record->name );
	}

	radicon_taurus_ptr =
		(MX_RADICON_TAURUS *) radicon_taurus_record->record_type_struct;

	if ( radicon_taurus_ptr == (MX_RADICON_TAURUS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RADICON_TAURUS pointer for record '%s' "
		"used by RS232 record '%s' is NULL.",
			radicon_taurus_record->name,
			rs232->record->name );
	}

	if ( radicon_taurus != (MX_RADICON_TAURUS **) NULL ) {
		*radicon_taurus = radicon_taurus_ptr;
	}

	if ( serial_port_record != (MX_RECORD **) NULL ) {
		*serial_port_record = radicon_taurus_ptr->serial_port_record;

		if ( (*serial_port_record) == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The serial_port_record pointer for area detector '%s' "
			"used by RS232 record '%s' is NULL.",
				radicon_taurus_record->name,
				rs232->record->name );
		}
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
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL.");
	}

#if MXI_RADICON_TAURUS_RS232_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, record->name));
#endif

	rs232 = (MX_RS232 *) record->record_class_struct;

	mx_status = mxi_radicon_taurus_rs232_get_pointers( rs232,
						NULL, NULL, NULL, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Remove any terminator characters specified in the database file. */

	rs232->read_terminator_array[0] = '\0';
	rs232->write_terminator_array[0] = '\0';

	mx_status = mx_rs232_convert_terminator_characters( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_radicon_taurus_rs232_getchar( MX_RS232 *rs232, char *c )
{
	static const char fname[] = "mxi_radicon_taurus_rs232_getchar()";

	MX_RECORD *serial_port_record = NULL;
	mx_status_type mx_status;

	mx_status = mxi_radicon_taurus_rs232_get_pointers( rs232,
					NULL, NULL, &serial_port_record, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_getchar( serial_port_record,
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

	MX_RECORD *serial_port_record = NULL;
	mx_status_type mx_status;

	mx_status = mxi_radicon_taurus_rs232_get_pointers( rs232,
					NULL, NULL, &serial_port_record, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_RADICON_TAURUS_RS232_DEBUG
	MX_DEBUG(-2, ("%s: sending 0x%x, '%c' to '%s'",
		fname, c, c, rs232->record->name));
#endif

	mx_status = mx_rs232_putchar( serial_port_record,
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

	MX_RADICON_TAURUS *radicon_taurus = NULL;
	mx_status_type mx_status;

	mx_status = mxi_radicon_taurus_rs232_get_pointers( rs232,
					NULL, &radicon_taurus, NULL, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_radicon_taurus_command( radicon_taurus, NULL,
					buffer, max_bytes_to_read,
					MXI_RADICON_TAURUS_RS232_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( bytes_read != (size_t *) NULL ) {
		*bytes_read = strlen( buffer ) + 1;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_radicon_taurus_rs232_putline( MX_RS232 *rs232,
				char *buffer,
				size_t *bytes_written )
{
	static const char fname[] = "mxi_radicon_taurus_rs232_putline()";

	MX_RADICON_TAURUS *radicon_taurus = NULL;
	mx_status_type mx_status;

	mx_status = mxi_radicon_taurus_rs232_get_pointers( rs232,
					NULL, &radicon_taurus, NULL, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_radicon_taurus_command( radicon_taurus, buffer,
					NULL, 0,
					MXI_RADICON_TAURUS_RS232_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( bytes_written != (size_t *) NULL ) {
		*bytes_written = strlen( buffer ) + 1;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_radicon_taurus_rs232_num_input_bytes_available( MX_RS232 *rs232 )
{
	static const char fname[] =
		"mxi_radicon_taurus_rs232_num_input_bytes_available()";

	MX_RECORD *serial_port_record = NULL;
	MX_CLOCK_TICK timeout_in_ticks, current_tick, finish_tick;
	int comparison;
	mx_status_type mx_status;

#if MXI_RADICON_TAURUS_RS232_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.",
			fname, rs232->record->name));
#endif

	mx_status = mxi_radicon_taurus_rs232_get_pointers( rs232,
					NULL, NULL, &serial_port_record, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* The detector often sends bare '>' or LF characters without any normal
	 * line termination after the end of responses.  This makes parsing the
	 * responses a bit more difficult.  What we do here to deal with this is
	 * to loop for up to timeout time from 'rs232->timeout'.  If the timeout
	 * time is reached, but there is still only 1 character in the read
	 * buffer, then we assume that this is just a bare '>' or LF and tell
	 * the caller that no bytes are available.
	 */

	timeout_in_ticks = mx_convert_seconds_to_clock_ticks( rs232->timeout );

	current_tick = mx_current_clock_tick();

	finish_tick = mx_add_clock_ticks( current_tick, timeout_in_ticks );

	while (1) {
		/* How many bytes can be read without blocking? */

		mx_status = mx_rs232_num_input_bytes_available( serial_port_record,
						&(rs232->num_input_bytes_available) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* If more than 1 byte is available, then break out of the while() loop. */

		if ( rs232->num_input_bytes_available > 1 ) {
			break;
		}

		current_tick = mx_current_clock_tick();

		comparison = mx_compare_clock_ticks( current_tick, finish_tick );

		if ( comparison >= 0 ) {
			/* We hae timed out, so set num_input_bytes_available to 0
			 * and then break out of the while() loop.
			 */

			rs232->num_input_bytes_available = 0;
			break;
		}
	}

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

	MX_RECORD *serial_port_record = NULL;
	mx_status_type mx_status;

	mx_status = mxi_radicon_taurus_rs232_get_pointers( rs232,
					NULL, NULL, &serial_port_record, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_discard_unread_input( serial_port_record,
					MXI_RADICON_TAURUS_RS232_DEBUG );

	return mx_status;
}

