/*
 * Name:    i_network_gpib.c
 *
 * Purpose: MX driver for network GPIB devices.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2005-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_NETWORK_GPIB_DEBUG		FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_gpib.h"
#include "mx_net.h"
#include "i_network_gpib.h"

MX_RECORD_FUNCTION_LIST mxi_network_gpib_record_function_list = {
	NULL,
	mxi_network_gpib_create_record_structures,
	mxi_network_gpib_finish_record_initialization,
	NULL,
	NULL,
	NULL,
	NULL,
	mxi_network_gpib_open,
	NULL,
	NULL,
	mxi_network_gpib_resynchronize
};

MX_GPIB_FUNCTION_LIST mxi_network_gpib_gpib_function_list = {
	mxi_network_gpib_open_device,
	mxi_network_gpib_close_device,
	mxi_network_gpib_read,
	mxi_network_gpib_write,
	mxi_network_gpib_interface_clear,
	mxi_network_gpib_device_clear,
	mxi_network_gpib_selective_device_clear,
	mxi_network_gpib_local_lockout,
	mxi_network_gpib_remote_enable,
	mxi_network_gpib_go_to_local,
	mxi_network_gpib_trigger,
	mxi_network_gpib_wait_for_service_request,
	mxi_network_gpib_serial_poll,
	mxi_network_gpib_serial_poll_disable
};

MX_RECORD_FIELD_DEFAULTS mxi_network_gpib_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_GPIB_STANDARD_FIELDS,
	MXI_NETWORK_GPIB_STANDARD_FIELDS
};

mx_length_type mxi_network_gpib_num_record_fields
		= sizeof( mxi_network_gpib_record_field_defaults )
			/ sizeof( mxi_network_gpib_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_network_gpib_rfield_def_ptr
			= &mxi_network_gpib_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxi_network_gpib_get_pointers( MX_GPIB *gpib,
			MX_NETWORK_GPIB **network_gpib,
			const char *calling_fname )
{
	static const char fname[] = "mxi_network_gpib_get_pointers()";

	if ( gpib == (MX_GPIB *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_GPIB pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( network_gpib == (MX_NETWORK_GPIB **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_NETWORK_GPIB pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( gpib->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_RECORD pointer for the "
			"MX_GPIB pointer passed by '%s' is NULL.",
			calling_fname );
	}

	*network_gpib = (MX_NETWORK_GPIB *) gpib->record->record_type_struct;

	if ( *network_gpib == (MX_NETWORK_GPIB *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_NETWORK_GPIB pointer for record '%s' is NULL.",
			gpib->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

#if MXI_NETWORK_GPIB_DEBUG

static void
mxi_network_gpib_display_binary( const char *fname,
				MX_GPIB *gpib,
				int32_t address,
				char *buffer,
				long max_bytes_to_display,
				long bytes_transferred,
				int is_write_operation )
{
	/* Display binary data as hex. */
		
	char debug_msg_buffer[100];
	char *ptr;
	long i, bytes_to_display;

	max_bytes_to_display = 10;

	if ( is_write_operation ) {
		sprintf( debug_msg_buffer, "%s: sent ", fname );
	} else {
		sprintf( debug_msg_buffer, "%s: received ", fname );
	}

	if ( bytes_transferred > max_bytes_to_display ) {
		bytes_to_display = max_bytes_to_display;
	} else {
		bytes_to_display = bytes_transferred;
	}

	for ( i = 0; i < bytes_to_display; i++ ) {
		ptr = debug_msg_buffer + strlen( debug_msg_buffer );

		sprintf( ptr, "%#x ", buffer[i] );
	}

	if ( bytes_transferred > max_bytes_to_display ) {
		ptr = debug_msg_buffer + strlen( debug_msg_buffer );

		sprintf( ptr, "... " );
	}

	ptr = debug_msg_buffer + strlen( debug_msg_buffer );

	if ( is_write_operation ) {
		sprintf( ptr, "to GPIB interface '%s', device %d",
			gpib->record->name, address );
	} else {
		sprintf( ptr, "from GPIB interface '%s', device %d",
			gpib->record->name, address );
	}

	MX_DEBUG(-2,("%s", debug_msg_buffer));
}

#endif /* MXI_NETWORK_GPIB_DEBUG */

/*==========================*/

MX_EXPORT mx_status_type
mxi_network_gpib_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxi_network_gpib_create_record_structures()";

	MX_GPIB *gpib;
	MX_NETWORK_GPIB *network_gpib;

	/* Allocate memory for the necessary structures. */

	gpib = (MX_GPIB *) malloc( sizeof(MX_GPIB) );

	if ( gpib == (MX_GPIB *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_GPIB structure." );
	}

	network_gpib = (MX_NETWORK_GPIB *) malloc( sizeof(MX_NETWORK_GPIB) );

	if ( network_gpib == (MX_NETWORK_GPIB *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_NETWORK_GPIB structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = gpib;
	record->record_type_struct = network_gpib;
	record->class_specific_function_list
				= &mxi_network_gpib_gpib_function_list;

	gpib->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_network_gpib_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxi_network_gpib_finish_record_initialization()";

	MX_GPIB *gpib;
	MX_NETWORK_GPIB *network_gpib;
	mx_status_type mx_status;

#if MXI_NETWORK_GPIB_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL.");
	}

	gpib = (MX_GPIB *) record->record_class_struct;

	mx_status = mxi_network_gpib_get_pointers( gpib, &network_gpib, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Initialize the network record field structures. */

	mx_network_field_init( &(network_gpib->address_nf),
		network_gpib->server_record,
		"%s.address", network_gpib->remote_record_name );

	mx_network_field_init( &(network_gpib->bytes_read_nf),
		network_gpib->server_record,
		"%s.bytes_read", network_gpib->remote_record_name );

	mx_network_field_init( &(network_gpib->bytes_to_read_nf),
		network_gpib->server_record,
		"%s.bytes_to_read", network_gpib->remote_record_name );

	mx_network_field_init( &(network_gpib->bytes_to_write_nf),
		network_gpib->server_record,
		"%s.bytes_to_write", network_gpib->remote_record_name );

	mx_network_field_init( &(network_gpib->bytes_written_nf),
		network_gpib->server_record,
		"%s.bytes_written", network_gpib->remote_record_name );

	mx_network_field_init( &(network_gpib->close_device_nf),
		network_gpib->server_record,
		"%s.close_device", network_gpib->remote_record_name );

	mx_network_field_init( &(network_gpib->device_clear_nf),
		network_gpib->server_record,
		"%s.device_clear", network_gpib->remote_record_name );

	mx_network_field_init( &(network_gpib->go_to_local_nf),
		network_gpib->server_record,
		"%s.go_to_local", network_gpib->remote_record_name );

	mx_network_field_init( &(network_gpib->interface_clear_nf),
		network_gpib->server_record,
		"%s.interface_clear", network_gpib->remote_record_name );

	mx_network_field_init( &(network_gpib->local_lockout_nf),
		network_gpib->server_record,
		"%s.local_lockout", network_gpib->remote_record_name );

	mx_network_field_init( &(network_gpib->open_device_nf),
		network_gpib->server_record,
		"%s.open_device", network_gpib->remote_record_name );

	mx_network_field_init( &(network_gpib->read_nf),
		network_gpib->server_record,
		"%s.read", network_gpib->remote_record_name );

	mx_network_field_init( &(network_gpib->remote_enable_nf),
		network_gpib->server_record,
		"%s.remote_enable", network_gpib->remote_record_name );

	mx_network_field_init( &(network_gpib->resynchronize_nf),
		network_gpib->server_record,
		"%s.resynchronize", network_gpib->remote_record_name );

	mx_network_field_init( &(network_gpib->selective_device_clear_nf),
		network_gpib->server_record,
		"%s.selective_device_clear", network_gpib->remote_record_name );

	mx_network_field_init( &(network_gpib->serial_poll_nf),
		network_gpib->server_record,
		"%s.serial_poll", network_gpib->remote_record_name );

	mx_network_field_init( &(network_gpib->serial_poll_disable_nf),
		network_gpib->server_record,
		"%s.serial_poll_disable", network_gpib->remote_record_name );

	mx_network_field_init( &(network_gpib->trigger_nf),
		network_gpib->server_record,
		"%s.trigger", network_gpib->remote_record_name );

	mx_network_field_init( &(network_gpib->wait_for_service_request_nf),
		network_gpib->server_record,
		"%s.wait_for_service_request",
		network_gpib->remote_record_name );

	mx_network_field_init( &(network_gpib->write_nf),
		network_gpib->server_record,
		"%s.write", network_gpib->remote_record_name );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_network_gpib_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_network_gpib_open()";

	MX_GPIB *gpib;
	MX_NETWORK_GPIB *network_gpib;
	char rfname[ MXU_RECORD_FIELD_NAME_LENGTH + 1 ];
	mx_status_type mx_status;

#if MXI_NETWORK_GPIB_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL.");
	}

	gpib = (MX_GPIB *) record->record_class_struct;

	mx_status = mxi_network_gpib_get_pointers( gpib, &network_gpib, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sprintf( rfname, "%s.read_buffer_length",
			network_gpib->remote_record_name );

	mx_status = mx_get_by_name( network_gpib->server_record,
				rfname, MXFT_LENGTH,
				&(network_gpib->remote_read_buffer_length) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sprintf( rfname, "%s.write_buffer_length",
			network_gpib->remote_record_name );

	mx_status = mx_get_by_name( network_gpib->server_record,
				rfname, MXFT_LENGTH,
				&(network_gpib->remote_write_buffer_length) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_gpib_setup_communication_buffers( record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_network_gpib_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxi_network_gpib_resynchronize()";

	MX_GPIB *gpib;
	MX_NETWORK_GPIB *network_gpib;
	mx_bool_type resynchronize;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	gpib = (MX_GPIB *) record->record_class_struct;

	mx_status = mxi_network_gpib_get_pointers( gpib, &network_gpib, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	resynchronize = TRUE;

	mx_status = mx_put( &(network_gpib->resynchronize_nf),
				MXFT_BOOL, &resynchronize );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_network_gpib_open_device( MX_GPIB *gpib, int32_t address )
{
	static const char fname[] = "mxi_network_gpib_open_device()";

	MX_NETWORK_GPIB *network_gpib;
	mx_status_type mx_status;

	mx_status = mxi_network_gpib_get_pointers( gpib, &network_gpib, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_NETWORK_GPIB_DEBUG
	MX_DEBUG(-2,("%s: opening GPIB interface '%s', device %d.",
			fname, gpib->record->name, address ));
#endif

	mx_status = mx_put( &(network_gpib->open_device_nf),
					MXFT_INT32, &address );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_network_gpib_close_device( MX_GPIB *gpib, int32_t address )
{
	static const char fname[] = "mxi_network_gpib_close_device()";

	MX_NETWORK_GPIB *network_gpib;
	mx_status_type mx_status;

	mx_status = mxi_network_gpib_get_pointers( gpib, &network_gpib, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_NETWORK_GPIB_DEBUG
	MX_DEBUG(-2,("%s: closing GPIB interface '%s', address %d.",
			fname, gpib->record->name, address ));
#endif

	mx_status = mx_put( &(network_gpib->close_device_nf),
					MXFT_INT32, &address );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_network_gpib_read( MX_GPIB *gpib,
			int32_t address,
			char *buffer,
			size_t max_bytes_to_read,
			size_t *bytes_read,
			mx_hex_type flags )
{
	static const char fname[] = "mxi_network_gpib_read()";

	MX_NETWORK_GPIB *network_gpib;
	int32_t local_bytes_to_read, local_bytes_read;
	mx_length_type dimension_array[1];
	mx_status_type mx_status;

	mx_status = mxi_network_gpib_get_pointers( gpib, &network_gpib, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the address and the number of bytes to read. */

	mx_status = mx_put( &(network_gpib->address_nf), MXFT_INT32, &address );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	local_bytes_to_read = (long) max_bytes_to_read;

	mx_status = mx_put( &(network_gpib->bytes_to_read_nf),
					MXFT_INT32, &local_bytes_to_read );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Read from the GPIB device. */

	if ( max_bytes_to_read > network_gpib->remote_read_buffer_length ) {
		max_bytes_to_read = network_gpib->remote_read_buffer_length;
	}

	dimension_array[0] = (long) max_bytes_to_read;

	mx_status = mx_get_array( &(network_gpib->read_nf),
				MXFT_CHAR, 1, dimension_array, buffer );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the number of bytes read. */

	mx_status = mx_get( &(network_gpib->bytes_read_nf),
					MXFT_INT32, &local_bytes_read );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( bytes_read != NULL ) {
		*bytes_read = local_bytes_read;
	}

#if MXI_NETWORK_GPIB_DEBUG
	if ( gpib->ascii_read ) {
		MX_DEBUG(-2,
		    ("%s: received '%s' from GPIB interface '%s', device %d.",
			 fname, buffer, gpib->record->name, address ));
	} else {
		mxi_network_gpib_display_binary( fname, gpib, address, buffer,
						10, local_bytes_read, FALSE );
	}
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_network_gpib_write( MX_GPIB *gpib,
			int32_t address,
			char *buffer,
			size_t bytes_to_write,
			size_t *bytes_written,
			mx_hex_type flags )
{
	static const char fname[] = "mxi_network_gpib_write()";

	MX_NETWORK_GPIB *network_gpib;
	int32_t local_bytes_to_write, local_bytes_written;
	mx_length_type dimension_array[1];
	mx_status_type mx_status;

	mx_status = mxi_network_gpib_get_pointers( gpib, &network_gpib, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the address and the number of bytes to write. */

	mx_status = mx_put( &(network_gpib->address_nf), MXFT_INT32, &address );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	local_bytes_to_write = (long) bytes_to_write;

	mx_status = mx_put( &(network_gpib->bytes_to_write_nf),
					MXFT_INT32, &local_bytes_to_write );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Write to the GPIB device. */

	if ( bytes_to_write > network_gpib->remote_write_buffer_length ) {
		bytes_to_write = network_gpib->remote_write_buffer_length;
	}

	dimension_array[0] = (long) bytes_to_write;

	mx_status = mx_put_array( &(network_gpib->write_nf),
				MXFT_CHAR, 1, dimension_array, buffer );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the number of bytes written. */

	mx_status = mx_get( &(network_gpib->bytes_written_nf),
					MXFT_INT32, &local_bytes_written );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( bytes_written != NULL ) {
		*bytes_written = local_bytes_written;
	}

#if MXI_NETWORK_GPIB_DEBUG
	if ( gpib->ascii_write ) {
		MX_DEBUG(-2,
		    ("%s: sent '%s' to GPIB interface '%s', device %d.",
			 fname, buffer, gpib->record->name, address ));
	} else {
		mxi_network_gpib_display_binary( fname, gpib, address, buffer,
						10, local_bytes_written, TRUE );
	}
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_network_gpib_interface_clear( MX_GPIB *gpib )
{
	static const char fname[] = "mxi_network_gpib_interface_clear()";

	MX_NETWORK_GPIB *network_gpib;
	mx_bool_type interface_clear;
	mx_status_type mx_status;

	mx_status = mxi_network_gpib_get_pointers( gpib, &network_gpib, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_NETWORK_GPIB_DEBUG
	MX_DEBUG(-2,("%s: interface clear for GPIB interface '%s'.",
			fname, gpib->record->name ));
#endif
	interface_clear = TRUE;

	mx_status = mx_put( &(network_gpib->interface_clear_nf),
						MXFT_INT32, &interface_clear );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_network_gpib_device_clear( MX_GPIB *gpib )
{
	static const char fname[] = "mxi_network_gpib_device_clear()";

	MX_NETWORK_GPIB *network_gpib;
	mx_bool_type device_clear;
	mx_status_type mx_status;

	mx_status = mxi_network_gpib_get_pointers( gpib, &network_gpib, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_NETWORK_GPIB_DEBUG
	MX_DEBUG(-2,("%s: device clear for GPIB device '%s'.",
			fname, gpib->record->name ));
#endif
	device_clear = TRUE;

	mx_status = mx_put( &(network_gpib->device_clear_nf),
						MXFT_BOOL, &device_clear );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_network_gpib_selective_device_clear( MX_GPIB *gpib, int32_t address )
{
	static const char fname[] = "mxi_network_gpib_selective_device_clear()";

	MX_NETWORK_GPIB *network_gpib;
	mx_status_type mx_status;

	mx_status = mxi_network_gpib_get_pointers( gpib, &network_gpib, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_NETWORK_GPIB_DEBUG
	MX_DEBUG(-2,
	("%s: selective device clear for GPIB device '%s', device %d.",
			fname, gpib->record->name, address ));
#endif

	mx_status = mx_put( &(network_gpib->selective_device_clear_nf),
						MXFT_INT32, &address );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_network_gpib_local_lockout( MX_GPIB *gpib )
{
	static const char fname[] = "mxi_network_gpib_local_lockout()";

	MX_NETWORK_GPIB *network_gpib;
	mx_bool_type local_lockout;
	mx_status_type mx_status;

	mx_status = mxi_network_gpib_get_pointers( gpib, &network_gpib, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_NETWORK_GPIB_DEBUG
	MX_DEBUG(-2,("%s: local lockout for GPIB device '%s'.",
			fname, gpib->record->name ));
#endif
	local_lockout = TRUE;

	mx_status = mx_put( &(network_gpib->local_lockout_nf),
						MXFT_BOOL, &local_lockout );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_network_gpib_remote_enable( MX_GPIB *gpib, int32_t address )
{
	static const char fname[] = "mxi_network_gpib_remote_enable()";

	MX_NETWORK_GPIB *network_gpib;
	mx_status_type mx_status;

	mx_status = mxi_network_gpib_get_pointers( gpib, &network_gpib, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_NETWORK_GPIB_DEBUG
	MX_DEBUG(-2,
	("%s: remote enable for GPIB device '%s', device %d.",
			fname, gpib->record->name, address ));
#endif

	mx_status = mx_put( &(network_gpib->remote_enable_nf),
						MXFT_INT32, &address );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_network_gpib_go_to_local( MX_GPIB *gpib, int32_t address )
{
	static const char fname[] = "mxi_network_gpib_go_to_local()";

	MX_NETWORK_GPIB *network_gpib;
	mx_status_type mx_status;

	mx_status = mxi_network_gpib_get_pointers( gpib, &network_gpib, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_NETWORK_GPIB_DEBUG
	MX_DEBUG(-2,
	("%s: go to local for GPIB device '%s', device %d.",
			fname, gpib->record->name, address ));
#endif

	mx_status = mx_put( &(network_gpib->go_to_local_nf),
						MXFT_INT32, &address );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_network_gpib_trigger( MX_GPIB *gpib, int32_t address )
{
	static const char fname[] = "mxi_network_gpib_trigger()";

	MX_NETWORK_GPIB *network_gpib;
	mx_status_type mx_status;

	mx_status = mxi_network_gpib_get_pointers( gpib, &network_gpib, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_NETWORK_GPIB_DEBUG
	MX_DEBUG(-2,("%s: trigger for GPIB device '%s', device %d.",
			fname, gpib->record->name, address ));
#endif

	mx_status = mx_put( &(network_gpib->trigger_nf), MXFT_INT32, &address );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_network_gpib_wait_for_service_request( MX_GPIB *gpib, double timeout )
{
	static const char fname[] =
		"mxi_network_gpib_wait_for_service_request()";

	MX_NETWORK_GPIB *network_gpib;
	mx_status_type mx_status;

	mx_status = mxi_network_gpib_get_pointers( gpib, &network_gpib, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_NETWORK_GPIB_DEBUG
	MX_DEBUG(-2,
	("%s: wait for service request for GPIB interface '%s', timeout = %g",
			fname, gpib->record->name, timeout ));
#endif
	mx_status = mx_put( &(network_gpib->wait_for_service_request_nf),
						MXFT_DOUBLE, &timeout );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_network_gpib_serial_poll( MX_GPIB *gpib,
				int32_t address,
				unsigned char *serial_poll_byte )
{
	static const char fname[] = "mxi_network_gpib_serial_poll()";

	MX_NETWORK_GPIB *network_gpib;
	uint8_t local_serial_poll_byte;
	mx_status_type mx_status;

	mx_status = mxi_network_gpib_get_pointers( gpib, &network_gpib, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the GPIB address. */

	mx_status = mx_put( &(network_gpib->address_nf), MXFT_INT32, &address );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Serial poll the selected device. */

	mx_status = mx_put( &(network_gpib->serial_poll_nf),
					MXFT_UINT8, &local_serial_poll_byte );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_NETWORK_GPIB_DEBUG
	MX_DEBUG(-2,
	("%s: serial poll for GPIB device '%s', device %d returned %#x.",
		fname, gpib->record->name, address, local_serial_poll_byte ));
#endif

	if ( serial_poll_byte != NULL ) {
		*serial_poll_byte = local_serial_poll_byte;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_network_gpib_serial_poll_disable( MX_GPIB *gpib )
{
	static const char fname[] = "mxi_network_gpib_serial_poll_disable()";

	MX_NETWORK_GPIB *network_gpib;
	mx_bool_type serial_poll_disable;
	mx_status_type mx_status;

	mx_status = mxi_network_gpib_get_pointers( gpib, &network_gpib, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_NETWORK_GPIB_DEBUG
	MX_DEBUG(-2,("%s: serial poll disable for GPIB device '%s'.",
			fname, gpib->record->name ));
#endif
	serial_poll_disable = TRUE;

	mx_status = mx_put( &(network_gpib->serial_poll_disable_nf),
					MXFT_BOOL, &serial_poll_disable );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

