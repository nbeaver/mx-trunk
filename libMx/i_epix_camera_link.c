/*
 * Name:    i_epix_camera_link.c
 *
 * Purpose: MX driver for National Instruments CAMERA_LINK interfaces.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_EPIX_CAMERA_LINK_DEBUG	TRUE

#include <stdio.h>

#include "mxconfig.h"

#if HAVE_EPIX_XCLIB 

#include <stdlib.h>

#if defined(OS_WIN32)
#   include <windows.h>
#endif

#include "mx_util.h"
#include "mx_record.h"
#include "mx_hrt.h"
#include "mx_camera_link.h"
#include "i_epix_xclib.h"
#include "i_epix_camera_link.h"

/* Include the vendor include file. */

#include "xcliball.h"

MX_RECORD_FUNCTION_LIST mxi_epix_camera_link_record_function_list = {
	NULL,
	mxi_epix_camera_link_create_record_structures,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxi_epix_camera_link_open,
	mxi_epix_camera_link_close
};

MX_CAMERA_LINK_API_LIST mxi_epix_camera_link_api_list = {
	NULL,
	NULL,
	NULL,
	mxi_epix_camera_link_get_num_bytes_avail,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxi_epix_camera_link_serial_close,
	mxi_epix_camera_link_serial_init,
	mxi_epix_camera_link_serial_read,
	mxi_epix_camera_link_serial_write,
	mxi_epix_camera_link_set_baud_rate,
	mxi_epix_camera_link_set_cc_line
};

MX_RECORD_FIELD_DEFAULTS mxi_epix_camera_link_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_CAMERA_LINK_STANDARD_FIELDS,
	MXI_EPIX_CAMERA_LINK_STANDARD_FIELDS
};

long mxi_epix_camera_link_num_record_fields
		= sizeof( mxi_epix_camera_link_record_field_defaults )
			/ sizeof( mxi_epix_camera_link_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_epix_camera_link_rfield_def_ptr
			= &mxi_epix_camera_link_record_field_defaults[0];

/*-----*/

static MX_EPIX_CAMERA_LINK_PORT **mxi_epix_camera_link_serial_port_array = NULL;

static int mxi_epix_camera_link_num_serial_ports = 0;

/*-----*/

static mx_status_type
mxi_epix_camera_link_get_pointers( MX_CAMERA_LINK *camera_link,
			MX_EPIX_CAMERA_LINK **epix_camera_link,
			MX_CAMERA_LINK_API_LIST **api_list_ptr,
			const char *calling_fname )
{
	static const char fname[] = "mxi_epix_camera_link_get_pointers()";

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

	if ( epix_camera_link != (MX_EPIX_CAMERA_LINK **) NULL ) {
		*epix_camera_link = record->record_type_struct;
	}

	if ( api_list_ptr != (MX_CAMERA_LINK_API_LIST **) NULL ) {
		*api_list_ptr = record->class_specific_function_list;
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ==== Public functions ==== */

MX_EXPORT mx_status_type
mxi_epix_camera_link_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxi_epix_camera_link_create_record_structures()";

	MX_CAMERA_LINK *camera_link;
	MX_EPIX_CAMERA_LINK *epix_camera_link;

	/* Allocate memory for the necessary structures. */

	camera_link = (MX_CAMERA_LINK *) malloc( sizeof(MX_CAMERA_LINK) );

	if ( camera_link == (MX_CAMERA_LINK *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for an MX_CAMERA_LINK structure." );
	}

	epix_camera_link = (MX_EPIX_CAMERA_LINK *)
				malloc( sizeof(MX_EPIX_CAMERA_LINK) );

	if ( epix_camera_link == (MX_EPIX_CAMERA_LINK *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for an MX_EPIX_CAMERA_LINK structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = camera_link;
	record->record_type_struct = epix_camera_link;
	record->record_function_list =
				&mxi_epix_camera_link_record_function_list;
	record->class_specific_function_list = &mxi_epix_camera_link_api_list;

	camera_link->record = record;
	epix_camera_link->record = record;
	epix_camera_link->camera_link = camera_link;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_epix_camera_link_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_epix_camera_link_open()";

	MX_CAMERA_LINK *camera_link;
	MX_EPIX_CAMERA_LINK *epix_camera_link;
	MX_EPIX_CAMERA_LINK_PORT *port;
	MX_CAMERA_LINK_API_LIST *api_list;
	int i, num_ports;
	mx_status_type mx_status;

	/* Suppress GCC 4 uninitialized variable warning. */

	epix_camera_link = NULL;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	camera_link = (MX_CAMERA_LINK *) record->record_class_struct;

	mx_status = mxi_epix_camera_link_get_pointers( camera_link,
					&epix_camera_link, &api_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	epix_camera_link->unitmap = 1 << (camera_link->serial_index - 1);

#if MXI_EPIX_CAMERA_LINK_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s', unitmap = %#lx",
		fname, record->name, epix_camera_link->unitmap));
#endif

	/* Is this port already in the list of Camera Link serial ports? */

	for ( i = 0; i < mxi_epix_camera_link_num_serial_ports; i++ ) {
		port = mxi_epix_camera_link_serial_port_array[i];

		if ( port == NULL ) {
			/* Skip this one. */

#if MXI_EPIX_CAMERA_LINK_DEBUG
			MX_DEBUG(-2,("%s: i = %d, port = NULL", fname, i));
#endif
			continue;
		}

		if ( port->unit_number == camera_link->serial_index ) {

			if ( port->epix_camera_link == epix_camera_link ) {
				/* This port is already in the list,
				 * so return.
				 */

#if MXI_EPIX_CAMERA_LINK_DEBUG
				MX_DEBUG(-2,
				("%s: Record '%s', unit number %ld is already "
				"in the list at element %d.",
					fname, record->name,
					camera_link->serial_index, i ));
#endif

				return MX_SUCCESSFUL_RESULT;
			} else {
				return mx_error( MXE_NOT_AVAILABLE, fname,
				"EPIX serial port %ld requested by record '%s' "
				"is already in use by record '%s'.",
					camera_link->serial_index,
					record->name,
					port->epix_camera_link->record->name );
			}
		}
	}

#if MXI_EPIX_CAMERA_LINK_DEBUG
	MX_DEBUG(-2,("%s: i = %d", fname, i));
#endif

	/* The port is not already in the list, so add it to the list. */

	port = malloc( sizeof(MX_EPIX_CAMERA_LINK_PORT) );

	if ( port == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to allocate an "
			"MX_EPIX_CAMERA_LINK_PORT structure." );
	}

	port->unit_number = camera_link->serial_index;
	port->array_index = i;
	port->in_use = FALSE;
	port->record = record;
	port->camera_link = camera_link;
	port->epix_camera_link = epix_camera_link;

	mxi_epix_camera_link_num_serial_ports = i+1;

	num_ports = mxi_epix_camera_link_num_serial_ports;

	if ( mxi_epix_camera_link_serial_port_array == NULL ) {
		mxi_epix_camera_link_serial_port_array = malloc(
			num_ports * sizeof( MX_EPIX_CAMERA_LINK_PORT * ) );
	} else {
		mxi_epix_camera_link_serial_port_array = realloc(
			mxi_epix_camera_link_serial_port_array,
			num_ports * sizeof( MX_EPIX_CAMERA_LINK_PORT * ) );
	}

	if ( mxi_epix_camera_link_serial_port_array == NULL ) {
		mxi_epix_camera_link_num_serial_ports = 0;

		return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to change the size of the "
			"EPIX camera link serial port array to %d elements.  "
			"Camera Link serial I/O may not operate correctly "
			"after this error.", i );
	}

	mxi_epix_camera_link_serial_port_array[i] = port;

#if MXI_EPIX_CAMERA_LINK_DEBUG
	MX_DEBUG(-2,("%s: Record '%s', unit number %ld has been added "
		"to the array at element %d.",
		fname, record->name, camera_link->serial_index, i));
	MX_DEBUG(-2,("%s: mxi_epix_camera_link_num_serial_ports = %d",
		fname, mxi_epix_camera_link_num_serial_ports));
#endif

	mx_status = mx_camera_link_open( record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_epix_camera_link_close( MX_RECORD *record )
{
	static const char fname[] = "mxi_epix_camera_link_close()";

	MX_EPIX_CAMERA_LINK_PORT *port;
	int i;

	if ( mxi_epix_camera_link_serial_port_array == NULL ) {
		mxi_epix_camera_link_num_serial_ports = 0;

		return MX_SUCCESSFUL_RESULT;
	}

	if ( mxi_epix_camera_link_num_serial_ports == 0 ) {
		mxi_epix_camera_link_serial_port_array = NULL;

		return MX_SUCCESSFUL_RESULT;
	}

	for ( i = 0; i < mxi_epix_camera_link_num_serial_ports; i++ ) {
		port = mxi_epix_camera_link_serial_port_array[i];

		if ( port != NULL ) {
			free( port );
		}
	}

	free( mxi_epix_camera_link_serial_port_array );

	mxi_epix_camera_link_num_serial_ports = 0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT INT32 MX_CLCALL
mxi_epix_camera_link_get_num_bytes_avail( hSerRef serial_ref,
					UINT32 *num_bytes )
{
	static const char fname[] =
		"mxi_epix_camera_link_get_num_bytes_avail()";

	MX_EPIX_CAMERA_LINK_PORT *port;
	long bytes_available;
	char error_message[80];

	if ( serial_ref == NULL ) {
		return CL_ERR_INVALID_REFERENCE;
	}

	port = serial_ref;

#if MXI_EPIX_CAMERA_LINK_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, port->record->name));
#endif

	/* How many bytes can be read without blocking? */

	bytes_available =
		   pxd_serialRead(port->epix_camera_link->unitmap, 0, NULL, 0);

	if ( bytes_available < 0 ) {
		mxi_epix_xclib_error_message(
			port->epix_camera_link->unitmap, bytes_available,
			error_message, sizeof(error_message) );

		(void) mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"EPIX error code %ld returned while attempting "
		"to determine the number of characters in the "
		"receive buffer for Camera Link interface '%s'.  %s",
			bytes_available, port->record->name,
			error_message );

		return CL_ERR_ERROR_NOT_FOUND;
	}

	*num_bytes = bytes_available;

	return CL_ERR_NO_ERR;
}

MX_EXPORT void MX_CLCALL
mxi_epix_camera_link_serial_close( hSerRef serial_ref )
{
	static const char fname[] = "mxi_epix_camera_link_serial_close()";

	MX_EPIX_CAMERA_LINK_PORT *port;

	if ( serial_ref == NULL ) {

#if MXI_EPIX_CAMERA_LINK_DEBUG
		MX_DEBUG(-2,("%s invoked for a NULL pointer.", fname));
#endif
		return;
	}

	port = serial_ref;

#if MXI_EPIX_CAMERA_LINK_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, port->record->name));
#endif

	/* Mark the port as not in use. */

	port->in_use = FALSE;

	return;
}

MX_EXPORT INT32 MX_CLCALL
mxi_epix_camera_link_serial_init( UINT32 serial_index, hSerRef *serial_ref_ptr )
{
	static const char fname[] = "mxi_epix_camera_link_serial_init()";

	MX_EPIX_CAMERA_LINK_PORT *port;
	int i;

#if MXI_EPIX_CAMERA_LINK_DEBUG
	MX_DEBUG(-2,("%s invoked for serial index %lu.",
		fname, (unsigned long) serial_index ));
	MX_DEBUG(-2,("%s: mxi_epix_camera_link_num_serial_ports = %d",
		fname, mxi_epix_camera_link_num_serial_ports));
	MX_DEBUG(-2,("%s: mxi_epix_camera_link_serial_port_array = %p",
		fname, mxi_epix_camera_link_serial_port_array));
#endif

	if ( mxi_epix_camera_link_serial_port_array == NULL ) {
		return CL_ERR_ERROR_NOT_FOUND;
	}

	/* Find the port in the serial port array.  The 'serial_ref_ptr'
	 * will be a pointer to the MX_EPIX_CAMERA_LINK_PORT structure.
	 */

	for ( i = 0; i < mxi_epix_camera_link_num_serial_ports; i++ ) {
		port = mxi_epix_camera_link_serial_port_array[i];

		if ( port == NULL ) {
			/* Skip this one. */

			continue;
		}

#if MXI_EPIX_CAMERA_LINK_DEBUG
		MX_DEBUG(-2,("%s: i = %d, unit_number = %ld, array_index = %d, "
			"in_use = %d, record = %p, camera_link = %p, "
			"epix_camera_link = %p, record name = '%s'.",
			fname, i, port->unit_number, port->array_index,
			port->in_use, port->record, port->camera_link,
			port->epix_camera_link, port->record->name ));
#endif

		if ( port->unit_number == serial_index ) {
			/* We have found the port. */

			if ( port->in_use ) {

#if MXI_EPIX_CAMERA_LINK_DEBUG
			MX_DEBUG(-2,
		("%s: serial port '%s' (serial index %lu) is already in use.",
		fname, port->record->name, (unsigned long) serial_index ));
#endif
				return CL_ERR_PORT_IN_USE;
			}

			port->in_use = TRUE;

			*serial_ref_ptr = port;

#if MXI_EPIX_CAMERA_LINK_DEBUG
			MX_DEBUG(-2,
			("%s: serial index %lu is serial port '%s'.",
		fname, (unsigned long) serial_index, port->record->name ));
#endif
			return CL_ERR_NO_ERR;
		}
	}

	/* We did not find the port. */

#if MXI_EPIX_CAMERA_LINK_DEBUG
	MX_DEBUG(-2,("%s did not find a serial port for serial index %lu.",
		fname, (unsigned long) serial_index ));
#endif

	return CL_ERR_INVALID_INDEX;
}

MX_EXPORT INT32 MX_CLCALL
mxi_epix_camera_link_serial_read( hSerRef serial_ref, INT8 *buffer,
				UINT32 *num_bytes, UINT32 serial_timeout )
{
	static const char fname[] = "mxi_epix_camera_link_serial_read()";

	MX_EPIX_CAMERA_LINK_PORT *port;
	struct timespec start_time, timeout_time;
	struct timespec current_time, timeout_interval;
	long bytes_to_read, bytes_left, bytes_available, bytes_read;
	double timeout_in_seconds;
	int timeout_status;
	char *read_ptr;
	char error_message[80];

	if ( serial_ref == NULL ) {
		return CL_ERR_INVALID_REFERENCE;
	}

	port = serial_ref;

#if MXI_EPIX_CAMERA_LINK_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, port->record->name));
#endif

	/* Compute the timeout time in high resolution time units. */

	timeout_in_seconds = 0.001 * (double) serial_timeout;

	timeout_interval = 
	    mx_convert_seconds_to_high_resolution_time( timeout_in_seconds );

	start_time = mx_high_resolution_time();

	timeout_time = mx_add_high_resolution_times( start_time,
							timeout_interval );

	/* Loop until we read all of the bytes or else timeout. */

	bytes_left = *num_bytes;

	read_ptr = buffer;

	for (;;) {
		/* How many bytes can be read without blocking? */

		bytes_available =
		   pxd_serialRead(port->epix_camera_link->unitmap, 0, NULL, 0);

#if MXI_EPIX_CAMERA_LINK_DEBUG
		MX_DEBUG(-2,("%s: bytes_available = %ld",
			fname, bytes_available));
#endif

		if ( bytes_available < 0 ) {
			mxi_epix_xclib_error_message(
				port->epix_camera_link->unitmap,
				bytes_available, error_message,
				sizeof(error_message) );

			(void) mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"EPIX error code %ld returned while attempting "
			"to determine the number of characters in the "
			"receive buffer for Camera Link interface '%s'.  %s",
				bytes_available, port->record->name,
				error_message );

			return CL_ERR_ERROR_NOT_FOUND;
		}

		/* According to the Camera Link specification, we are not
		 * supposed to read any bytes unless we can read all of
		 * them in one operation.  However, the receive buffer is
		 * MX_EPIX_CAMERA_LINK_RECEIVE_BUFFER_SIZE bytes in length
		 * for EPIX Camera Link boards, so we must read a subset of
		 * the expected number of bytes anyway if the read buffer
		 * is full.
		 */

		if ( bytes_available >= bytes_left ) {
			bytes_to_read = bytes_left;
		} else
		if (bytes_available >= MX_EPIX_CAMERA_LINK_RECEIVE_BUFFER_SIZE){
			bytes_to_read = MX_EPIX_CAMERA_LINK_RECEIVE_BUFFER_SIZE;
		} else {
			bytes_to_read = 0;
		}

		/* Read as many characters as we can.*/

		if ( bytes_to_read > 0 ) {
			bytes_read = pxd_serialRead(
					port->epix_camera_link->unitmap, 0,
					read_ptr, bytes_to_read );

#if MXI_EPIX_CAMERA_LINK_DEBUG
			MX_DEBUG(-2,("%s: pxd_serialRead() bytes_read = %ld",
				fname, bytes_read));
#endif

			if ( bytes_read < 0 ) {
				mxi_epix_xclib_error_message(
				port->epix_camera_link->unitmap, bytes_read,
				error_message, sizeof(error_message) );

				(void) mx_error( MXE_INTERFACE_IO_ERROR, fname,
				"EPIX error code %ld returned while attempting "
				"to read from Camera Link interface '%s'.  %s",
					bytes_read, port->record->name,
					error_message );

				return CL_ERR_ERROR_NOT_FOUND;
			}

#if MXI_EPIX_CAMERA_LINK_DEBUG
			{
				int i;

				for ( i = 0; i < bytes_read; i++ ) {
					MX_DEBUG(-2,("%s: read_ptr[%d] = %#x",
						fname, i, read_ptr[i]));
				}
			}
#endif

			bytes_left -= bytes_read;

			read_ptr += bytes_read;
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

#if MXI_EPIX_CAMERA_LINK_DEBUG
			MX_DEBUG(-2,
			("%s: Timed out after waiting %g seconds "
			"to read to Camera Link interface '%s'.",
			    fname, timeout_in_seconds, port->record->name ));
#endif

			return CL_ERR_TIMEOUT;
		}

		/* Give the operating system a chance to do something else. */

		mx_usleep(1);	/* Sleep for >= 1 microsecond. */
	}

	return CL_ERR_NO_ERR;
}

MX_EXPORT INT32 MX_CLCALL
mxi_epix_camera_link_serial_write( hSerRef serial_ref, INT8 *buffer,
				UINT32 *num_bytes, UINT32 serial_timeout )
{
	static const char fname[] = "mxi_epix_camera_link_serial_write()";

	MX_EPIX_CAMERA_LINK_PORT *port;
	struct timespec start_time, timeout_time;
	struct timespec current_time, timeout_interval;
	long bytes_to_write, bytes_left, transmit_buffer_available_space;
	double timeout_in_seconds;
	int epix_status, timeout_status;
	char *write_ptr;
	char error_message[80];

	if ( serial_ref == NULL ) {
		return CL_ERR_INVALID_REFERENCE;
	}

	port = serial_ref;

#if MXI_EPIX_CAMERA_LINK_DEBUG
	MX_DEBUG(-2,
		("%s invoked for record '%s'.", fname, port->record->name));

	{
		int i;

		for ( i = 0; i < *num_bytes; i++ ) {
			MX_DEBUG(-2,("%s: buffer[%d] = %#x",
				fname, i, buffer[i]));
		}
	}
#endif

	/* Compute the timeout time in high resolution time units. */

	timeout_in_seconds = 0.001 * (double) serial_timeout;

	timeout_interval = 
	    mx_convert_seconds_to_high_resolution_time( timeout_in_seconds );

	start_time = mx_high_resolution_time();

	timeout_time = mx_add_high_resolution_times( start_time,
							timeout_interval );

	/* Loop until we write all of the bytes or else timeout. */

	bytes_left = *num_bytes;

	write_ptr = buffer;

	for (;;) {
		/* How many bytes can be written without blocking? */

		transmit_buffer_available_space = pxd_serialWrite(
					port->epix_camera_link->unitmap,
					0, NULL, 0);

#if MXI_EPIX_CAMERA_LINK_DEBUG
		MX_DEBUG(-2,("%s: transmit_buffer_available_space = %ld",
			fname, transmit_buffer_available_space));
#endif

		if ( transmit_buffer_available_space < 0 ) {
			mxi_epix_xclib_error_message(
			port->epix_camera_link->unitmap,
			transmit_buffer_available_space,
			error_message, sizeof(error_message) );

			(void) mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"EPIX error code %ld returned while attempting "
			"to determine the available transmit buffer space "
			"for Camera Link interface '%s'.  %s",
				transmit_buffer_available_space,
				port->record->name, error_message );

			return CL_ERR_ERROR_NOT_FOUND;
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
			epix_status = pxd_serialWrite(
					port->epix_camera_link->unitmap, 0,
					write_ptr, bytes_to_write );

#if MXI_EPIX_CAMERA_LINK_DEBUG
			MX_DEBUG(-2,("%s: pxd_serialWrite() epix_status = %d",
				fname, epix_status));
#endif
			if ( epix_status < 0 ) {
				mxi_epix_xclib_error_message(
				port->epix_camera_link->unitmap, epix_status,
				error_message, sizeof(error_message) );

				(void) mx_error( MXE_INTERFACE_IO_ERROR, fname,
				"EPIX error code %d returned while attempting "
				"to write to Camera Link interface '%s'.  %s",
					epix_status, port->record->name,
					error_message );

				return CL_ERR_ERROR_NOT_FOUND;
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

#if MXI_EPIX_CAMERA_LINK_DEBUG
			MX_DEBUG(-2,
			("%s: Timed out after waiting %g seconds "
			"to write to Camera Link interface '%s'.",
			    fname, timeout_in_seconds, port->record->name ));
#endif

			return CL_ERR_TIMEOUT;
		}

		/* Give the operating system a chance to do something else. */

		mx_usleep(1);	/* Sleep for >= 1 microsecond. */
	}

	return CL_ERR_NO_ERR;
}

MX_EXPORT INT32 MX_CLCALL
mxi_epix_camera_link_set_baud_rate( hSerRef serial_ref, UINT32 baud_rate )
{
	static const char fname[] = "mxi_epix_camera_link_set_baud_rate()";

	MX_EPIX_CAMERA_LINK_PORT *port;
	int epix_status;
	char error_message[80];

	if ( serial_ref == NULL ) {
		return CL_ERR_INVALID_REFERENCE;
	}

	port = serial_ref;

#if MXI_EPIX_CAMERA_LINK_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, port->record->name));
#endif

	epix_status = pxd_serialConfigure( port->epix_camera_link->unitmap, 0,
			(double) baud_rate, 8, 0, 1, 0, 0, 0 );

	switch( epix_status ) {
	case 0:		/* Success */
		break;

	case PXERNOTOPEN:
		(void) mx_error( MXE_INITIALIZATION_ERROR, fname,
		"The EPIX XCLIB library has not yet successfully been "
		"initialized by pxd_PIXCIopen()." );

		return CL_ERR_ERROR_NOT_FOUND;
		break;

	default:
		mxi_epix_xclib_error_message(
			port->epix_camera_link->unitmap, epix_status,
			error_message, sizeof(error_message) );

		(void) mx_error( MXE_DEVICE_IO_ERROR, fname,
		"pxd_serialConfigure( %#lx, 0, %g, 8, 0, 1, 0, 0, 0 ) failed "
		"with epix_status = %d.  %s",
	    		port->epix_camera_link->unitmap, (double) baud_rate,
			epix_status, error_message );

		return CL_ERR_ERROR_NOT_FOUND;
	}

	return CL_ERR_NO_ERR;
}

MX_EXPORT INT32 MX_CLCALL
mxi_epix_camera_link_set_cc_line( hSerRef serial_ref,
					UINT32 cc_line_number,
					UINT32 cc_line_state )
{
	static const char fname[] = "mxi_epix_camera_link_set_cc_line()";

	char error_message[80];

	MX_EPIX_CAMERA_LINK_PORT *port;
	uint16 CLCCSE;
	int epix_status;

	struct xclibs *xc;
	xclib_DeclareVidStateStructs(vidstate);

	if ( serial_ref == NULL ) {
		return CL_ERR_INVALID_REFERENCE;
	}

	port = serial_ref;

#if MXI_EPIX_CAMERA_LINK_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, port->record->name));
#endif

#if MXD_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s: cc_line_number = %d, cc_line_state = %d",
			fname, cc_line_number, cc_line_state));
#endif

	xc = pxd_xclibEscape(0, 0, 0);

	if ( xc == NULL ) {
		(void) mx_error( MXE_INITIALIZATION_ERROR, fname,
		"The XCLIB library has not yet been initialized "
		"for Camera Link interface '%s' with pxd_PIXCIopen().",
			port->record->name );

		return CL_ERR_ERROR_NOT_FOUND;
	}

	xclib_InitVidStateStructs(vidstate);

	xc->pxlib.getState( &(xc->pxlib), 0, PXMODE_DIGI, &vidstate );

	CLCCSE = vidstate.xc.dxxformat->CLCCSE;

#if MXD_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s: Old CLCCSE = %#x", fname, CLCCSE));
#endif

	switch( cc_line_state ) {
	case 1:   /* High */

		switch( cc_line_number ) {
		case MX_CAMERA_LINK_CC1:
			/* CC1 is controlled by bits 0 to 3. */

			/* CLCCSE &= 0xfff0; */
			CLCCSE |= 0x0001;
			break;

		case MX_CAMERA_LINK_CC2:
			/* CC2 is controlled by bits 4 to 7. */

			CLCCSE &= 0xff0f;
			CLCCSE |= 0x0010;
			break;

		case MX_CAMERA_LINK_CC3:
			/* CC3 is controlled by bits 8 to 11. */

			CLCCSE &= 0xf0ff;
			CLCCSE |= 0x0100;
			break;

		case MX_CAMERA_LINK_CC4:
			/* CC4 is controlled by bits 12 to 15. */

			CLCCSE &= 0x0fff;
			CLCCSE |= 0x1000;
			break;

		default:
			(void) mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Illegal Camera Link line %d was requested "
			"for Camera Link interface '%s'.", cc_line_number,
				port->record->name );

			return CL_ERR_INVALID_INDEX;
		}
		break;

	case 0:   /* Low */

		switch( cc_line_number ) {
		case MX_CAMERA_LINK_CC1:
			/* CC1 is controlled by bits 0 to 3. */

			CLCCSE &= 0xfff0;
			break;

		case MX_CAMERA_LINK_CC2:
			/* CC2 is controlled by bits 4 to 7. */

			CLCCSE &= 0xff0f;
			break;

		case MX_CAMERA_LINK_CC3:
			/* CC3 is controlled by bits 8 to 11. */

			CLCCSE &= 0xf0ff;
			break;

		case MX_CAMERA_LINK_CC4:
			/* CC4 is controlled by bits 12 to 15. */

			CLCCSE &= 0x0fff;
			break;

		default:
			(void) mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Illegal Camera Link line %d was requested "
			"for Camera Link interface '%s'.", cc_line_number,
				port->record->name );

			return CL_ERR_INVALID_INDEX;
		}
		break;

	default:
		(void) mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal line state %d was requested for Camera Link line CC%d "
		"of Camera Link interface '%s'.",
			cc_line_state, cc_line_number,
			port->record->name );

		return CL_ERR_ERROR_NOT_FOUND;
	}

#if MXD_EPIX_XCLIB_DEBUG
	MX_DEBUG(-2,("%s: New CLCCSE = %#x", fname, CLCCSE));
#endif

	vidstate.xc.dxxformat->CLCCSE = CLCCSE;

	epix_status = xc->pxlib.defineState(
				&(xc->pxlib), 0, PXMODE_DIGI, &vidstate );

	if ( epix_status != 0 ) {
		(void) mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Error in xc->pxlib.defineState() for record '%s'.  "
		"Error code = %d", port->record->name, epix_status );

		return CL_ERR_ERROR_NOT_FOUND;
	}

	epix_status = pxd_xclibEscaped(0, 0, 0);

	if ( epix_status != 0 ) {
		(void) mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Error in pxd_xclibEscaped() for record '%s'.  "
		"Error code = %d", port->record->name, epix_status );

		return CL_ERR_ERROR_NOT_FOUND;
	}


	return CL_ERR_NO_ERR;
}

#endif /* HAVE_EPIX_XCLIB */

