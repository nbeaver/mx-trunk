/*
 * Name:    i_prologix.c
 *
 * Purpose: MX driver for the Prologix GPIB-USB and GPIB-Ethernet controllers
 *          used in System Controller mode.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2009 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_PROLOGIX_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_unistd.h"
#include "mx_hrt.h"
#include "mx_gpib.h"
#include "mx_rs232.h"
#include "mx_driver.h"
#include "i_prologix.h"

MX_RECORD_FUNCTION_LIST mxi_prologix_record_function_list = {
	NULL,
	mxi_prologix_create_record_structures,
	mx_gpib_finish_record_initialization,
	NULL,
	NULL,
	NULL,
	NULL,
	mxi_prologix_open
};

MX_GPIB_FUNCTION_LIST mxi_prologix_gpib_function_list = {
	mxi_prologix_open_device,
	mxi_prologix_close_device,
	mxi_prologix_read,
	mxi_prologix_write,
	mxi_prologix_interface_clear,
	mxi_prologix_device_clear,
	mxi_prologix_selective_device_clear,
	mxi_prologix_local_lockout,
	mxi_prologix_remote_enable,
	mxi_prologix_go_to_local,
	mxi_prologix_trigger,
	mxi_prologix_wait_for_service_request,
	mxi_prologix_serial_poll,
	mxi_prologix_serial_poll_disable
};

MX_RECORD_FIELD_DEFAULTS mxi_prologix_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_GPIB_STANDARD_FIELDS,
	MXI_PROLOGIX_STANDARD_FIELDS
};

long mxi_prologix_num_record_fields
		= sizeof( mxi_prologix_record_field_defaults )
			/ sizeof( mxi_prologix_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_prologix_rfield_def_ptr
			= &mxi_prologix_record_field_defaults[0];

/* ==== Private function for the driver's use only. ==== */

static mx_status_type
mxi_prologix_get_pointers( MX_GPIB *gpib,
				MX_PROLOGIX **prologix,
				const char *calling_fname )
{
	static const char fname[] = "mxi_prologix_get_pointers()";

	if ( gpib == (MX_GPIB *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_GPIB pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( prologix == (MX_PROLOGIX **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_PROLOGIX pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*prologix = (MX_PROLOGIX *) (gpib->record->record_type_struct);

	if ( *prologix == (MX_PROLOGIX *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_PROLOGIX pointer for record '%s' is NULL.",
			gpib->record->name );
	}

	if ( (*prologix)->rs232_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The RS-232 MX_RECORD pointer for record '%s' is NULL.",
			gpib->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxi_prologix_update_address( MX_PROLOGIX *prologix,
				long address,
				int debug )
{
	char command[30];
	mx_status_type mx_status;

	if ( address != prologix->current_address ) {

		snprintf( command, sizeof(command), "++addr %ld", address );

		mx_status = mx_rs232_putline( prologix->rs232_record,
					command, NULL, debug );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	prologix->current_address = address;

	return MX_SUCCESSFUL_RESULT;
}

/* ==== Public functions ==== */

MX_EXPORT mx_status_type
mxi_prologix_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_prologix_create_record_structures()";

	MX_GPIB *gpib;
	MX_PROLOGIX *prologix = NULL;

	/* Allocate memory for the necessary structures. */

	gpib = (MX_GPIB *) malloc( sizeof(MX_GPIB) );

	if ( gpib == (MX_GPIB *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_GPIB structure." );
	}

	prologix = (MX_PROLOGIX *) malloc( sizeof(MX_PROLOGIX) );

	if ( prologix == (MX_PROLOGIX *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_PROLOGIX structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = gpib;
	record->record_type_struct = prologix;
	record->class_specific_function_list
				= &mxi_prologix_gpib_function_list;

	gpib->record = record;

	prologix->current_address = -1;

	prologix->escaped_buffer_length = 100;
	prologix->escaped_buffer = malloc( prologix->escaped_buffer_length );

	if ( prologix->escaped_buffer == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to create a %ld byte "
		"escaped command buffer for GPIB interface '%s'.",
			prologix->escaped_buffer_length,
			record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_prologix_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_prologix_open()";

	MX_GPIB *gpib;
	MX_PROLOGIX *prologix = NULL;
	MX_RS232 *rs232;
	char response[80];
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	gpib = (MX_GPIB *) record->record_class_struct;

	mx_status = mxi_prologix_get_pointers( gpib, &prologix, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	rs232 = (MX_RS232 *) prologix->rs232_record->record_class_struct;

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_RS232 pointer for RS-232 record '%s' is NULL.",
			prologix->rs232_record->name );
	}

	/* If possible, discard any unwritten characters. */

	(void) mx_rs232_discard_unwritten_output( prologix->rs232_record,
						MXI_PROLOGIX_DEBUG );

	/* Configure the Prologix interface to be a System Controller. */

	mx_status = mx_rs232_putline( prologix->rs232_record,
					"++mode 1",
					NULL, MXI_PROLOGIX_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Turn on automatic Read-After-Write. */

	mx_status = mx_rs232_putline( prologix->rs232_record,
					"++auto 1",
					NULL, MXI_PROLOGIX_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the read timeout to 1000 milliseconds. */

	mx_status = mx_rs232_putline( prologix->rs232_record,
					"++read_tmo_ms 1000",
					NULL, MXI_PROLOGIX_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Enable EOI on last character. */

	mx_status = mx_rs232_putline( prologix->rs232_record,
					"++eoi 1",
					NULL, MXI_PROLOGIX_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Do not append a character when EOI is detected during a read. */

	mx_status = mx_rs232_putline( prologix->rs232_record,
					"++eot_enable 0",
					NULL, MXI_PROLOGIX_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* FIXME - Investigate further about GPIB termination characters. */

	/* Set the GPIB termination character to LF. */

	mx_status = mx_rs232_putline( prologix->rs232_record,
					"++eos 2",
					NULL, MXI_PROLOGIX_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If possible, discard any garbage characters that may be
	 * in the input buffer.
	 */

	(void) mx_rs232_discard_unread_input( prologix->rs232_record,
						MXI_PROLOGIX_DEBUG );

	/* Ask the Prologix for its version number. */

	mx_status = mx_rs232_putline( prologix->rs232_record, "++ver",
					NULL, MXI_PROLOGIX_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_getline( prologix->rs232_record,
					response, sizeof(response),
					NULL, MXI_PROLOGIX_DEBUG );

	MX_DEBUG(-2,("%s: Prologix version = '%s'", fname, response));

	gpib->read_buffer_length = 0;
	gpib->write_buffer_length = 0;

	mx_status = mx_gpib_setup_communication_buffers( record );

	return mx_status;
}

/* ========== Device specific calls ========== */

MX_EXPORT mx_status_type
mxi_prologix_open_device( MX_GPIB *gpib, long address )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_prologix_close_device( MX_GPIB *gpib, long address )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_prologix_read( MX_GPIB *gpib,
		long address,
		char *buffer,
		size_t max_bytes_to_read,
		size_t *bytes_read,
		unsigned long transfer_flags )
{
	static const char fname[] = "mxi_prologix_read()";

	MX_PROLOGIX *prologix = NULL;
	int debug;
	mx_status_type mx_status;

	mx_status = mxi_prologix_get_pointers( gpib, &prologix, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( transfer_flags & MXF_232_DEBUG ) {
		debug = TRUE;

		transfer_flags &= ( ~ MXF_232_DEBUG );
	} else {
		debug = FALSE;
	}

	mx_status = mxi_prologix_update_address( prologix, address, debug );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_getline( prologix->rs232_record,
					buffer, max_bytes_to_read,
					bytes_read, transfer_flags );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( debug ) {
		MX_DEBUG(-2,("%s: received '%s' from '%s'.",
				fname, buffer, prologix->record->name ));
	}

	return mx_status;
}

#define MX_PREFIX_BUFFER_LENGTH   10

MX_EXPORT mx_status_type
mxi_prologix_write( MX_GPIB *gpib,
		long address,
		char *buffer,
		size_t bytes_to_write,
		size_t *bytes_written,
		unsigned long transfer_flags )
{
	static const char fname[] = "mxi_prologix_write()";

	MX_PROLOGIX *prologix = NULL;
	int debug;
	mx_status_type mx_status;

	mx_status = mxi_prologix_get_pointers( gpib, &prologix, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( transfer_flags & MXF_232_DEBUG ) {
		debug = TRUE;

		transfer_flags &= ( ~ MXF_232_DEBUG );
	} else {
		debug = FALSE;
	}

	if ( debug ) {
		MX_DEBUG(-2,("%s: sending '%s' to '%s'.",
			fname, buffer, prologix->record->name ));
	}

	mx_status = mxi_prologix_update_address( prologix, address, debug );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_putline( prologix->rs232_record,
				buffer, bytes_written, transfer_flags );

	return mx_status;
}

#undef MX_PREFIX_BUFFER_LENGTH

MX_EXPORT mx_status_type
mxi_prologix_interface_clear( MX_GPIB *gpib )
{
	static const char fname[] = "mxi_prologix_interface_clear()";

	MX_PROLOGIX *prologix = NULL;
	mx_status_type mx_status;

	mx_status = mxi_prologix_get_pointers( gpib, &prologix, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	prologix->current_address = -1;

	mx_status = mx_rs232_putline( prologix->rs232_record,
					"++ifc",
					NULL, MXI_PROLOGIX_DEBUG );
	return mx_status;
}

MX_EXPORT mx_status_type
mxi_prologix_device_clear( MX_GPIB *gpib )
{
	int i;

	/* Simulate a device clear by doing a selective device clear
	 * to all devices on the GPIB bus.
	 */

	for ( i = 0; i <= 30; i++ ) {
		(void) mxi_prologix_selective_device_clear( gpib, i );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_prologix_selective_device_clear( MX_GPIB *gpib, long address )
{
	static const char fname[] = "mxi_prologix_selective_device_clear()";

	MX_PROLOGIX *prologix = NULL;
	mx_status_type mx_status;

	mx_status = mxi_prologix_get_pointers( gpib, &prologix, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_prologix_update_address( prologix, address,
						MXI_PROLOGIX_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_putline( prologix->rs232_record,
					"++clr",
					NULL, MXI_PROLOGIX_DEBUG );
	return mx_status;
}

MX_EXPORT mx_status_type
mxi_prologix_local_lockout( MX_GPIB *gpib )
{
	static const char fname[] = "mxi_prologix_local_lockout()";

	return mx_error( MXE_UNSUPPORTED, fname,
	"Local lockout is not supported by GPIB interface '%s'.",
		gpib->record->name );
}

MX_EXPORT mx_status_type
mxi_prologix_remote_enable( MX_GPIB *gpib, long address )
{
	static const char fname[] = "mxi_prologix_remote_enable()";

	return mx_error( MXE_UNSUPPORTED, fname,
	"Remote enable is not supported by GPIB interface '%s'.",
		gpib->record->name );
}

MX_EXPORT mx_status_type
mxi_prologix_go_to_local( MX_GPIB *gpib, long address )
{
	static const char fname[] = "mxi_prologix_go_to_local()";

	MX_PROLOGIX *prologix = NULL;
	mx_status_type mx_status;

	mx_status = mxi_prologix_get_pointers( gpib, &prologix, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( address < 0 ) {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Go to local for address %ld is not supported "
		"for GPIB interface '%s'.",
			address, gpib->record->name );
	}

	mx_status = mxi_prologix_update_address( prologix, address,
							MXI_PROLOGIX_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_putline( prologix->rs232_record,
					"++loc",
					NULL, MXI_PROLOGIX_DEBUG );
	return mx_status;
}

MX_EXPORT mx_status_type
mxi_prologix_trigger( MX_GPIB *gpib, long address )
{
	static const char fname[] = "mxi_prologix_trigger_device()";

	MX_PROLOGIX *prologix = NULL;
	char command[40];
	mx_status_type mx_status;

	MX_DEBUG( 2, ("%s invoked.", fname));

	mx_status = mxi_prologix_get_pointers( gpib, &prologix, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Send the trigger command. */

	snprintf( command, sizeof(command), "++trg %ld", address );

	mx_status = mx_rs232_putline( prologix->rs232_record, command,
					NULL, MXI_PROLOGIX_DEBUG );
	return mx_status;
}

MX_EXPORT mx_status_type
mxi_prologix_wait_for_service_request( MX_GPIB *gpib, double timeout )
{
	static const char fname[] = "mxi_prologix_wait_for_service_request()";

	MX_PROLOGIX *prologix = NULL;
	struct timespec timeout_duration, current_time, finish_time;
	int num_items, srq_asserted, comparison;
	char response[40];
	mx_status_type mx_status;

	mx_status = mxi_prologix_get_pointers( gpib, &prologix, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Compute the timeout time in 'struct timespec' units. */

	timeout_duration = mx_convert_seconds_to_high_resolution_time(timeout);

	current_time = mx_high_resolution_time();

	finish_time = mx_add_high_resolution_times( current_time,
							timeout_duration );

	/* Loop waiting for the SRQ signal to be asserted. */

	while ( 1 ) {
		/* Get the status of the SRQ line. */

		mx_status = mx_rs232_putline( prologix->rs232_record,
						"++srq",
						NULL, MXI_PROLOGIX_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_rs232_getline( prologix->rs232_record,
						response, sizeof(response),
						NULL, MXI_PROLOGIX_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "%d", &srq_asserted );

		if ( num_items != 1 ) {
			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"The SRQ signal status was not found in the "
			"response '%s' to the '++srq' command sent to "
			"GPIB interface '%s'.",
				response, gpib->record->name );
		}

		if ( srq_asserted ) {
			return MX_SUCCESSFUL_RESULT;
		}

		/* See if the timeout time has arrived. */

		current_time = mx_high_resolution_time();

		comparison = mx_compare_high_resolution_times( current_time,
								finish_time );

		/* If the timeout time has arrived, then break
		 * out of the while loop.
		 */

		if ( comparison >= 0 ) {
			break;
		}

		mx_msleep(10);
	}

	return mx_error( MXE_TIMED_OUT, fname,
		"Timed out after waiting %g seconds for the SRQ line "
		"to be asserted by GPIB interface '%s'.",
			timeout, gpib->record->name );
}

MX_EXPORT mx_status_type
mxi_prologix_serial_poll( MX_GPIB *gpib, long address,
				unsigned char *serial_poll_byte)
{
	static const char fname[] = "mxi_prologix_serial_poll()";

	MX_PROLOGIX *prologix = NULL;
	char command[20];
	char response[20];
	unsigned short short_value;
	int num_items;
	mx_status_type mx_status;

	MX_DEBUG( 2, ("%s invoked.", fname));

	mx_status = mxi_prologix_get_pointers( gpib, &prologix, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the serial poll byte. */

	snprintf( command, sizeof(command), "++spoll %ld", address );

	mx_status = mx_rs232_putline( prologix->rs232_record,
					command,
					NULL, MXI_PROLOGIX_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_getline( prologix->rs232_record,
					response, sizeof(response),
					NULL, MXI_PROLOGIX_DEBUG );

	num_items = sscanf( response, "%hu", &short_value );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Serial poll byte not seen in response '%s' to command '%s'",
			response, command );
	}

	*serial_poll_byte = (unsigned char) ( short_value & 0xff );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_prologix_serial_poll_disable( MX_GPIB *gpib )
{
	/* Apparently this function is not needed for this interface. */

	return MX_SUCCESSFUL_RESULT;
}

