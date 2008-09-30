/*
 * Name:    i_epix_rs232.c
 *
 * Purpose: MX driver to communicate with the serial port of an
 *          EPIX, Inc. video input board.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_EPIX_RS232_DEBUG	TRUE

#include <stdio.h>

#include "mxconfig.h"

#if HAVE_EPIX_XCLIB

#include <stdlib.h>
#include <ctype.h>

#if defined(OS_WIN32)
#   include <windows.h>
#endif

#include "mx_util.h"
#include "mx_record.h"
#include "mx_hrt.h"
#include "mx_rs232.h"

#include "xcliball.h"	/* Vendor include file. */

#include "i_epix_xclib.h"
#include "i_epix_rs232.h"

MX_RECORD_FUNCTION_LIST mxi_epix_rs232_record_function_list = {
	NULL,
	mxi_epix_rs232_create_record_structures,
	mxi_epix_rs232_finish_record_initialization,
	NULL,
	NULL,
	NULL,
	NULL,
	mxi_epix_rs232_open
};

MX_RS232_FUNCTION_LIST mxi_epix_rs232_rs232_function_list = {
	mxi_epix_rs232_getchar,
	mxi_epix_rs232_putchar,
	mxi_epix_rs232_read,
	mxi_epix_rs232_write,
	NULL,
	NULL,
	mxi_epix_rs232_num_input_bytes_available,
	mxi_epix_rs232_discard_unread_input
};

MX_RECORD_FIELD_DEFAULTS mxi_epix_rs232_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_RS232_STANDARD_FIELDS,
	MXI_EPIX_RS232_STANDARD_FIELDS
};

long mxi_epix_rs232_num_record_fields
		= sizeof( mxi_epix_rs232_record_field_defaults )
			/ sizeof( mxi_epix_rs232_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_epix_rs232_rfield_def_ptr
			= &mxi_epix_rs232_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxi_epix_rs232_get_pointers( MX_RS232 *rs232,
			MX_EPIX_RS232 **epix_rs232,
			MX_EPIX_XCLIB **epix_xclib,
			const char *calling_fname )
{
	static const char fname[] = "mxi_epix_rs232_get_pointers()";

	MX_EPIX_RS232 *epix_rs232_ptr;
	MX_RECORD *xclib_record;

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RS232 pointer passed by '%s' was NULL.",
			calling_fname );
	}

	epix_rs232_ptr = (MX_EPIX_RS232 *)
				rs232->record->record_type_struct;

	if ( epix_rs232_ptr == (MX_EPIX_RS232 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_EPIX_RS232 pointer for record '%s' is NULL.",
			rs232->record->name );
	}

	if ( epix_rs232 != (MX_EPIX_RS232 **) NULL ) {
		*epix_rs232 = epix_rs232_ptr;
	}

	if ( epix_xclib != (MX_EPIX_XCLIB **) NULL ) {
		xclib_record = epix_rs232_ptr->xclib_record;

		if ( xclib_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The xclib_record pointer for record '%s' is NULL.",
				rs232->record->name );
		}

		*epix_xclib =
			(MX_EPIX_XCLIB *) xclib_record->record_type_struct;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*==========================*/

MX_EXPORT mx_status_type
mxi_epix_rs232_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxi_epix_rs232_create_record_structures()";

	MX_RS232 *rs232;
	MX_EPIX_RS232 *epix_rs232;

	/* Allocate memory for the necessary structures. */

	rs232 = (MX_RS232 *) malloc( sizeof(MX_RS232) );

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_RS232 structure." );
	}

	epix_rs232 =
		(MX_EPIX_RS232 *) malloc( sizeof(MX_EPIX_RS232) );

	if ( epix_rs232 == (MX_EPIX_RS232 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_EPIX_RS232 structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = rs232;
	record->record_type_struct = epix_rs232;
	record->class_specific_function_list
				= &mxi_epix_rs232_rs232_function_list;

	rs232->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_epix_rs232_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxi_epix_rs232_finish_record_initialization()";

	mx_status_type status;

#if MXI_EPIX_RS232_DEBUG
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

	/* Mark the epix_rs232 device as being closed. */

	record->record_flags |= ( ~MXF_REC_OPEN );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_epix_rs232_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_epix_rs232_open()";

	MX_RS232 *rs232;
	MX_EPIX_RS232 *epix_rs232;
	MX_EPIX_XCLIB *epix_xclib;
	int epix_status, parity;
	mx_status_type mx_status;
	char error_message[80];

	epix_rs232 = NULL;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL.");
	}

#if MXI_EPIX_RS232_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, record->name));
#endif

	rs232 = (MX_RS232 *) record->record_class_struct;

	mx_status = mxi_epix_rs232_get_pointers( rs232,
				&epix_rs232, &epix_xclib, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_convert_terminator_characters( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	epix_rs232->unitmap = 1 << epix_rs232->unit_number;

	if ( toupper( rs232->parity ) == 'N' ) {
		parity = 0;
	} else {
		parity = 1;
	}

	epix_status = pxd_serialConfigure( epix_rs232->unitmap, 0,
			(double) rs232->speed, rs232->word_size, parity,
			rs232->stop_bits, 0, 0, 0 );

        switch( epix_status ) {
        case 0:         /* Success */
                break;

        case PXERNOTOPEN:
                return mx_error( MXE_INITIALIZATION_ERROR, fname,
                	"The EPIX XCLIB library has not yet successfully been "
	                "initialized by pxd_PIXCIopen()." );
                break;

        default:
                mxi_epix_xclib_error_message(
                        epix_rs232->unitmap, epix_status,
                        error_message, sizeof(error_message) );

                return mx_error( MXE_DEVICE_IO_ERROR, fname,
                "pxd_serialConfigure( %#lx, 0, %g, %ld, %d, %ld, 0, 0, 0 ) "
		"failed with epix_status = %d.  %s",
                        epix_rs232->unitmap, (double) rs232->speed,
			rs232->word_size, parity, rs232->stop_bits,
                        epix_status, error_message );
		break;
        }

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_epix_rs232_getchar( MX_RS232 *rs232, char *c )
{
	static const char fname[] = "mxi_epix_rs232_getchar()";

	mx_status_type mx_status;

	mx_status = mxi_epix_rs232_read( rs232, c, 1, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_EPIX_RS232_DEBUG
	MX_DEBUG(-2, ("%s: received 0x%x, '%c' from '%s'",
			fname, *c, *c, rs232->record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_epix_rs232_putchar( MX_RS232 *rs232, char c )
{
	static const char fname[] = "mxi_epix_rs232_putchar()";

	mx_status_type mx_status;

#if MXI_EPIX_RS232_DEBUG
	MX_DEBUG(-2, ("%s: sending 0x%x, '%c' to '%s'",
		fname, c, c, rs232->record->name));
#endif

	mx_status = mxi_epix_rs232_write( rs232, &c, 1, NULL );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_epix_rs232_read( MX_RS232 *rs232,
				char *buffer,
				size_t max_bytes_to_read,
				size_t *actual_bytes_read )
{
	static const char fname[] = "mxi_epix_rs232_read()";

	MX_EPIX_RS232 *epix_rs232;
	struct timespec start_time, timeout_time;
	struct timespec current_time, timeout_interval;
	long bytes_to_read, bytes_left, bytes_available, bytes_read;
	int timeout_status;
	char *read_ptr;
	char error_message[80];
	mx_status_type mx_status;

	epix_rs232 = NULL;

	mx_status = mxi_epix_rs232_get_pointers( rs232,
						&epix_rs232, NULL, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( buffer == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The buffer pointer passed for record '%s' is NULL.",
			rs232->record->name );
	}

#if MXI_EPIX_RS232_DEBUG
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

		bytes_available = 
			pxd_serialRead( epix_rs232->unitmap, 0, NULL, 0 );

#if MXI_EPIX_RS232_DEBUG
		MX_DEBUG(-2,("%s: bytes_available = %ld",
			fname, bytes_available));
#endif

		if ( bytes_available < 0 ) {
			mxi_epix_xclib_error_message(
				epix_rs232->unitmap,
				bytes_available, error_message,
				sizeof(error_message) );

			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"EPIX error code %ld returned while attempting "
			"to determine the number of characters in the "
			"receive buffer for EPIX serial interface '%s'.  %s",
				bytes_available, rs232->record->name,
				error_message );
		}

		/* Figure out the number of bytes that can be read and
		 * which will fit in the buffer.
		 */

		if ( bytes_available >= bytes_left ) {
			bytes_to_read = bytes_left;
		} else
		if ( bytes_available >= max_bytes_to_read ) {
			bytes_to_read = max_bytes_to_read;
		} else
		if ( bytes_available >= MX_EPIX_RS232_RECEIVE_BUFFER_SIZE ) {
			bytes_to_read = MX_EPIX_RS232_RECEIVE_BUFFER_SIZE;
		} else {
			bytes_to_read = bytes_available;
		}

#if MXI_EPIX_RS232_DEBUG
		MX_DEBUG(-2,("%s: bytes_to_read = %ld", fname, bytes_to_read));
#endif

		/* Read as many characters as we can.*/

		if ( bytes_to_read > 0 ) {
			bytes_read = pxd_serialRead( epix_rs232->unitmap, 0,
						read_ptr, bytes_to_read );

#if MXI_EPIX_RS232_DEBUG
			MX_DEBUG(-2,("%s: pxd_serialRead() bytes_read = %ld",
				fname, bytes_read));
#endif

			if ( bytes_read < 0 ) {
				mxi_epix_xclib_error_message(
					epix_rs232->unitmap, bytes_read,
					error_message, sizeof(error_message) );

				return mx_error( MXE_INTERFACE_IO_ERROR, fname,
				"EPIX error code %ld returned while attempting "
				"to read from EPIX serial interface '%s'.  %s",
					bytes_read, rs232->record->name,
					error_message );
			}

#if MXI_EPIX_RS232_DEBUG
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

#if MXI_EPIX_RS232_DEBUG
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
			"from EPIX serial port '%s'.", rs232->timeout,
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
mxi_epix_rs232_write( MX_RS232 *rs232,
				char *buffer,
				size_t max_bytes_to_write,
				size_t *bytes_written )
{
	static const char fname[] = "mxi_epix_rs232_write()";

	MX_EPIX_RS232 *epix_rs232;
	struct timespec start_time, timeout_time;
	struct timespec current_time, timeout_interval;
	long bytes_to_write, bytes_left, transmit_buffer_available_space;
	int epix_status, timeout_status;
	char *write_ptr;
	char error_message[80];
	mx_status_type mx_status;

	epix_rs232 = NULL;

	mx_status = mxi_epix_rs232_get_pointers( rs232,
						&epix_rs232, NULL, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( buffer == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The buffer pointer passed for record '%s' is NULL.",
			rs232->record->name );
	}

#if MXI_EPIX_RS232_DEBUG
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

	/* Compute the timeout time in high resolution time units. */

	timeout_interval = 
	    mx_convert_seconds_to_high_resolution_time( rs232->timeout );

	start_time = mx_high_resolution_time();

	timeout_time = mx_add_high_resolution_times( start_time,
							timeout_interval );

	/* Loop until we write all of the bytes or else timeout. */

	bytes_left = max_bytes_to_write;

	write_ptr = buffer;

	for (;;) {
		/* How many bytes can be written without blocking? */

		transmit_buffer_available_space = pxd_serialWrite(
					epix_rs232->unitmap, 0, NULL, 0);

#if MXI_EPIX_RS232_DEBUG
		MX_DEBUG(-2,("%s: transmit_buffer_available_space = %ld",
			fname, transmit_buffer_available_space));
#endif

		if ( transmit_buffer_available_space < 0 ) {
			mxi_epix_xclib_error_message(
				epix_rs232->unitmap,
				transmit_buffer_available_space,
				error_message, sizeof(error_message) );

			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"EPIX error code %ld returned while attempting "
			"to determine the available transmit buffer space "
			"for EPIX serial interface '%s'.  %s",
				transmit_buffer_available_space,
				rs232->record->name, error_message );
		}

		if ( transmit_buffer_available_space >= bytes_left ) {
			bytes_to_write = bytes_left;
		} else
		if ( transmit_buffer_available_space > 0 ) {
			bytes_to_write = transmit_buffer_available_space;
		} else {
			bytes_to_write = 0;
		}

		/* Send as many characters as will fit in the transmit buffer.*/

		if ( bytes_to_write > 0 ) {
			epix_status = pxd_serialWrite( epix_rs232->unitmap, 0,
						write_ptr, bytes_to_write );

#if MXI_EPIX_RS232_DEBUG
			MX_DEBUG(-2,("%s: pxd_serialWrite() epix_status = %d",
				fname, epix_status));
#endif
			if ( epix_status < 0 ) {
				mxi_epix_xclib_error_message(
					epix_rs232->unitmap, epix_status,
					error_message, sizeof(error_message) );

				return mx_error( MXE_INTERFACE_IO_ERROR, fname,
				"EPIX error code %d returned while attempting "
				"to write to EPIX serial interface '%s'.  %s",
					epix_status, rs232->record->name,
					error_message );
			}

			bytes_left -= bytes_to_write;

			write_ptr += bytes_to_write;
		}

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
			"Timed out after waiting %g seconds to write %ld bytes "
			"to EPIX serial port '%s'.", rs232->timeout,
				(long) bytes_to_write, rs232->record->name );
		}

		/* Give the operating system a chance to do something else. */

		mx_usleep(1);	/* Sleep for >= 1 microsecond. */
	}

	if ( bytes_written != NULL ) {
		*bytes_written = bytes_to_write;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_epix_rs232_num_input_bytes_available( MX_RS232 *rs232 )
{
	static const char fname[] =
		"mxi_epix_rs232_num_input_bytes_available()";

	MX_EPIX_RS232 *epix_rs232;
	int bytes_available;
	mx_status_type mx_status;
	char error_message[80];

#if MXI_EPIX_RS232_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.",
			fname, rs232->record->name));
#endif

	epix_rs232 = NULL;

	mx_status = mxi_epix_rs232_get_pointers( rs232,
					&epix_rs232, NULL, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* How many bytes can be read without blocking? */

	bytes_available =
		   pxd_serialRead( epix_rs232->unitmap, 0, NULL, 0 );

	if ( bytes_available < 0 ) {
		mxi_epix_xclib_error_message(
			epix_rs232->unitmap, bytes_available,
			error_message, sizeof(error_message) );

		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"EPIX error code %d returned while attempting "
			"to determine the number of characters in the "
			"receive buffer for EPIX serial interface '%s'.  %s",
				bytes_available, rs232->record->name,
				error_message );
	}

	rs232->num_input_bytes_available = bytes_available;

#if MXI_EPIX_RS232_DEBUG
	fprintf( stderr, "*%ld*", (long) rs232->num_input_bytes_available );
#endif
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_epix_rs232_discard_unread_input( MX_RS232 *rs232 )
{
	static const char fname[] =
			"mxi_epix_rs232_discard_unread_input()";

	MX_EPIX_RS232 *epix_rs232;
	unsigned long bytes_available, bytes_to_read;
	mx_status_type mx_status;
	char buffer[ MX_EPIX_RS232_RECEIVE_BUFFER_SIZE ];

	mx_status = mxi_epix_rs232_get_pointers( rs232,
					&epix_rs232, NULL, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* EPIX serial ports do not natively support a flush operation,
	 * so we emulate it by reading the current contents of the
	 * receive buffer and throwing them away.
	 */

	for (;;) {
		mx_msleep(1);

		mx_status = mxi_epix_rs232_num_input_bytes_available( rs232 );

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

		mx_status = mxi_epix_rs232_read( rs232,
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

#endif /* HAVE_EPIX_XCLIB */

