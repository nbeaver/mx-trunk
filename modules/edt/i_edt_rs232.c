/*
 * Name:    i_edt_rs232.c
 *
 * Purpose: MX driver to communicate with the serial port of an
 *          EDT video input board.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2007-2008, 2010, 2012 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_EDT_RS232_DEBUG	TRUE

#include <stdio.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_hrt.h"
#include "mx_image.h"
#include "mx_rs232.h"
#include "mx_video_input.h"

#include "edtinc.h"	/* Vendor include file. */

#include "i_edt.h"
#include "d_edt.h"
#include "i_edt_rs232.h"

MX_RECORD_FUNCTION_LIST mxi_edt_rs232_record_function_list = {
	NULL,
	mxi_edt_rs232_create_record_structures,
	mxi_edt_rs232_finish_record_initialization,
	NULL,
	NULL,
	mxi_edt_rs232_open
};

MX_RS232_FUNCTION_LIST mxi_edt_rs232_rs232_function_list = {
	mxi_edt_rs232_getchar,
	mxi_edt_rs232_putchar,
	mxi_edt_rs232_read,
	mxi_edt_rs232_write,
	NULL,
	mxi_edt_rs232_putline,
	mxi_edt_rs232_num_input_bytes_available,
	mxi_edt_rs232_discard_unread_input
};

MX_RECORD_FIELD_DEFAULTS mxi_edt_rs232_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_RS232_STANDARD_FIELDS,
	MXI_EDT_RS232_STANDARD_FIELDS
};

long mxi_edt_rs232_num_record_fields
		= sizeof( mxi_edt_rs232_record_field_defaults )
			/ sizeof( mxi_edt_rs232_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_edt_rs232_rfield_def_ptr
			= &mxi_edt_rs232_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxi_edt_rs232_get_pointers( MX_RS232 *rs232,
			MX_EDT_RS232 **edt_rs232,
			MX_EDT_VIDEO_INPUT **edt_vinput,
			const char *calling_fname )
{
	static const char fname[] = "mxi_edt_rs232_get_pointers()";

	MX_EDT_RS232 *edt_rs232_ptr;
	MX_RECORD *edt_vinput_record;

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RS232 pointer passed by '%s' was NULL.",
			calling_fname );
	}

	edt_rs232_ptr = (MX_EDT_RS232 *)
				rs232->record->record_type_struct;

	if ( edt_rs232_ptr == (MX_EDT_RS232 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_EDT_RS232 pointer for record '%s' is NULL.",
			rs232->record->name );
	}

	if ( edt_rs232 != (MX_EDT_RS232 **) NULL ) {
		*edt_rs232 = edt_rs232_ptr;
	}

	if ( edt_vinput != (MX_EDT_VIDEO_INPUT **) NULL ) {
		edt_vinput_record = edt_rs232_ptr->edt_vinput_record;

		if ( edt_vinput_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The edt_vinput_record pointer for record '%s' is NULL.",
				rs232->record->name );
		}

		*edt_vinput = (MX_EDT_VIDEO_INPUT *)
				edt_vinput_record->record_type_struct;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*==========================*/

MX_EXPORT mx_status_type
mxi_edt_rs232_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxi_edt_rs232_create_record_structures()";

	MX_RS232 *rs232;
	MX_EDT_RS232 *edt_rs232;

	/* Allocate memory for the necessary structures. */

	rs232 = (MX_RS232 *) malloc( sizeof(MX_RS232) );

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_RS232 structure." );
	}

	edt_rs232 = (MX_EDT_RS232 *) malloc( sizeof(MX_EDT_RS232) );

	if ( edt_rs232 == (MX_EDT_RS232 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_EDT_RS232 structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = rs232;
	record->record_type_struct = edt_rs232;
	record->class_specific_function_list
				= &mxi_edt_rs232_rs232_function_list;

	rs232->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_edt_rs232_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxi_edt_rs232_finish_record_initialization()";

	mx_status_type status;

#if MXI_EDT_RS232_DEBUG
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

	/* Mark the edt_rs232 device as being closed. */

	record->record_flags |= ( ~MXF_REC_OPEN );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_edt_rs232_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_edt_rs232_open()";

	MX_RS232 *rs232;
	MX_EDT_VIDEO_INPUT *edt_vinput = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL.");
	}

#if MXI_EDT_RS232_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, record->name));
#endif

	rs232 = (MX_RS232 *) record->record_class_struct;

	mx_status = mxi_edt_rs232_get_pointers( rs232,
					NULL, &edt_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_convert_terminator_characters( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* For some cards, calling edt_reset_serial() will tell the camera
	 * to throw away any outstanding reads or writes and puts the 
	 * serial state machine in a known idle state.
	 */

	edt_reset_serial( edt_vinput->pdv_p );

	/* FIXME: Should we invoke pdv_set_baud() here?  The EDT manual
	 *        suggests that we rely on the EDT config file for this.
	 */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_edt_rs232_getchar( MX_RS232 *rs232, char *c )
{
	static const char fname[] = "mxi_edt_rs232_getchar()";

	mx_status_type mx_status;

	mx_status = mxi_edt_rs232_read( rs232, c, 1, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_EDT_RS232_DEBUG
	MX_DEBUG(-2, ("%s: received 0x%x, '%c' from '%s'",
			fname, *c, *c, rs232->record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_edt_rs232_putchar( MX_RS232 *rs232, char c )
{
	static const char fname[] = "mxi_edt_rs232_putchar()";

	mx_status_type mx_status;

#if MXI_EDT_RS232_DEBUG
	MX_DEBUG(-2, ("%s: sending 0x%x, '%c' to '%s'",
		fname, c, c, rs232->record->name));
#endif

	mx_status = mxi_edt_rs232_write( rs232, &c, 1, NULL );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_edt_rs232_read( MX_RS232 *rs232,
				char *buffer,
				size_t max_bytes_to_read,
				size_t *actual_bytes_read )
{
	static const char fname[] = "mxi_edt_rs232_read()";

	MX_EDT_VIDEO_INPUT *edt_vinput = NULL;
	struct timespec start_time, timeout_time;
	struct timespec current_time, timeout_interval;
	long bytes_to_read, bytes_left, bytes_available, bytes_read;
	int timeout_status;
	char *read_ptr;
	mx_status_type mx_status;

	mx_status = mxi_edt_rs232_get_pointers( rs232,
					NULL, &edt_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( buffer == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The buffer pointer passed for record '%s' is NULL.",
			rs232->record->name );
	}

#if MXI_EDT_RS232_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.",
		fname, rs232->record->name));
#endif

	/* Compute the timeout time in high resolution time units. */

	timeout_interval = 
	    mx_convert_seconds_to_high_resolution_time( rs232->timeout );

	start_time = mx_high_resolution_time();

	timeout_time = mx_add_high_resolution_times( start_time,
							timeout_interval );

	/* Loop until we read all of the bytes or else timeout. */

	bytes_left = max_bytes_to_read;

	read_ptr = buffer;

	for (;;) {
		/* How many bytes can be read without blocking? */

		mx_status = mxi_edt_rs232_num_input_bytes_available( rs232 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		bytes_available = rs232->num_input_bytes_available;

#if MXI_EDT_RS232_DEBUG
		MX_DEBUG(-2,("%s: bytes_available = %ld",
			fname, bytes_available));
#endif
		/* Figure out the number of bytes that can be read and
		 * which will fit in the buffer.
		 */

		if ( bytes_available >= bytes_left ) {
			bytes_to_read = bytes_left;
		} else
		if ( bytes_available >= max_bytes_to_read ) {
			bytes_to_read = max_bytes_to_read;
		} else {
			bytes_to_read = bytes_available;
		}

#if MXI_EDT_RS232_DEBUG
		MX_DEBUG(-2,("%s: bytes_to_read = %ld", fname, bytes_to_read));
#endif

		/* Read as many characters as we can.*/

		if ( bytes_to_read > 0 ) {
			bytes_read = pdv_serial_read( edt_vinput->pdv_p,
						read_ptr, bytes_to_read );

#if MXI_EDT_RS232_DEBUG
			MX_DEBUG(-2,("%s: pdv_serial_read() bytes_read = %ld",
				fname, bytes_read));
#endif

			if ( bytes_read < 0 ) {
				return mx_error( MXE_INTERFACE_IO_ERROR, fname,
				"EDT serial interface '%s' reported that it "
				"returned a negative number of bytes (%ld).  "
				"This should not be able to happen.",
					rs232->record->name, bytes_read );
			}

#if MXI_EDT_RS232_DEBUG
			{
				int i;

				for ( i = 0; i < bytes_read; i++ ) {
					MX_DEBUG(-2,
					("%s: read_ptr[%d] = %#x '%c'",
					fname, i, read_ptr[i], read_ptr[i]));
				}
			}
#endif

			bytes_left -= bytes_read;

			read_ptr += bytes_read;
		}

#if MXI_EDT_RS232_DEBUG
		MX_DEBUG(-2,("%s: bytes_left = %ld", fname, bytes_left));
#endif

		if ( bytes_left <= 0 ) {
			/* We are finished writing, so exit the for(;;) loop. */

			break;
		}

		/* Has the timeout time arrived? */

		current_time = mx_high_resolution_time();

		timeout_status = mx_compare_high_resolution_times( current_time,
								timeout_time );

		if ( timeout_status >= 0 ) {
			/* We have timed out. */

			return mx_error( MXE_TIMED_OUT, fname,
			"Timed out after waiting %g seconds to read %ld bytes "
			"from EDT serial port '%s'.", rs232->timeout,
				(long) max_bytes_to_read, rs232->record->name );
		}

		/* Give the operating system a chance to do something else. */

		mx_usleep(1);	/* Sleep for >= 1 microsecond. */
	}


	if ( actual_bytes_read != NULL ) {
		*actual_bytes_read = max_bytes_to_read;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_edt_rs232_write( MX_RS232 *rs232,
				char *buffer,
				size_t max_bytes_to_write,
				size_t *bytes_written )
{
	static const char fname[] = "mxi_edt_rs232_write()";

	MX_EDT_VIDEO_INPUT *edt_vinput = NULL;
	int edt_status;
	mx_status_type mx_status;

	mx_status = mxi_edt_rs232_get_pointers( rs232,
					NULL, &edt_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( buffer == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The buffer pointer passed for record '%s' is NULL.",
			rs232->record->name );
	}

#if MXI_EDT_RS232_DEBUG
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
	edt_status = pdv_serial_binary_command( edt_vinput->pdv_p,
						buffer, max_bytes_to_write );

#if MXI_EDT_RS232_DEBUG
	MX_DEBUG(-2,("%s: pdv_serial_binary_command() edt_status = %d",
			fname, edt_status));
#endif
	if ( edt_status != 0 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"EDT error code %d returned while attempting "
			"to write to EDT serial interface '%s'.",
				edt_status, rs232->record->name );
	}

	if ( bytes_written != NULL ) {
		*bytes_written = max_bytes_to_write;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_edt_rs232_putline( MX_RS232 *rs232,
				char *buffer,
				size_t *bytes_written )
{
	static const char fname[] = "mxi_edt_rs232_putline()";

	MX_EDT_VIDEO_INPUT *edt_vinput = NULL;
	int edt_status;
	mx_status_type mx_status;

	mx_status = mxi_edt_rs232_get_pointers( rs232,
					NULL, &edt_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( buffer == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The buffer pointer passed for record '%s' is NULL.",
			rs232->record->name );
	}

#if MXI_EDT_RS232_DEBUG
	MX_DEBUG(-2,("%s: sending '%s' to '%s'",
		fname, rs232->record->name, buffer));
#endif
	edt_status = pdv_serial_command( edt_vinput->pdv_p, buffer );

#if MXI_EDT_RS232_DEBUG
	MX_DEBUG(-2,("%s: pdv_serial_command() edt_status = %d",
			fname, edt_status));
#endif
	if ( edt_status != 0 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"EDT error code %d returned while attempting "
			"to write to EDT serial interface '%s'.",
				edt_status, rs232->record->name );
	}

	if ( bytes_written != NULL ) {
		*bytes_written = strlen(buffer)
			+ rs232->num_write_terminator_chars;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_edt_rs232_num_input_bytes_available( MX_RS232 *rs232 )
{
	static const char fname[] =
		"mxi_edt_rs232_num_input_bytes_available()";

	MX_EDT_VIDEO_INPUT *edt_vinput = NULL;
	int bytes_available;
	mx_status_type mx_status;

#if MXI_EDT_RS232_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.",
		fname, rs232->record->name));
#endif

	mx_status = mxi_edt_rs232_get_pointers( rs232,
					NULL, &edt_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* How many bytes can be read without blocking? */

	bytes_available = pdv_serial_wait( edt_vinput->pdv_p, 1, 1000000 );

	if ( bytes_available < 0 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"EDT serial interface '%s' says that there are "
			"a negative number (%d) of bytes available "
			"to be read.  This should not be able to happen.",
				rs232->record->name, bytes_available );
	}

	rs232->num_input_bytes_available = bytes_available;

#if MXI_EDT_RS232_DEBUG
	fprintf( stderr, "*%ld*", (long) rs232->num_input_bytes_available );
#endif
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_edt_rs232_discard_unread_input( MX_RS232 *rs232 )
{
	static const char fname[] =
			"mxi_edt_rs232_discard_unread_input()";

	MX_EDT_VIDEO_INPUT *edt_vinput = NULL;
	unsigned long bytes_available, bytes_to_read;
	mx_status_type mx_status;
	char buffer[100];

	mx_status = mxi_edt_rs232_get_pointers( rs232,
					NULL, &edt_vinput, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* EDT serial ports do not natively support a flush operation,
	 * so we emulate it by reading the current contents of the
	 * receive buffer and throwing them away.
	 */

	for (;;) {
		mx_msleep(1);

		mx_status = mxi_edt_rs232_num_input_bytes_available( rs232 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		bytes_available = rs232->num_input_bytes_available;

		if ( bytes_available > sizeof(buffer) ) {
			bytes_to_read = sizeof(buffer);
		} else
		if ( bytes_available > 0 ) {
			bytes_to_read = bytes_available;
		} else {
			/* All bytes have been read, so break
			 * out of the for(;;) loop.
			 */

			break;
		}

		mx_status = mxi_edt_rs232_read( rs232,
					buffer, bytes_to_read, NULL );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( bytes_available > bytes_to_read ) {
			bytes_available -= bytes_to_read;
		} else {
			bytes_available = 0;
		}
	}
	return mx_status;
}

