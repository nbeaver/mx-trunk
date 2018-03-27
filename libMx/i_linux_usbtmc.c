/*
 * Name:    i_linux_usbtmc.c
 *
 * Purpose: Linux MX GPIB driver for USBTMC devices controlled via
 *          /dev/usbtmc0 and friends.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2018 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_LINUX_USBTMC_DEBUG		TRUE

#include <stdio.h>

#include "mx_util.h"

#if defined( OS_LINUX ) && ( MX_GLIBC_VERSION >= 0L )

#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "mx_record.h"
#include "mx_gpib.h"
#include "i_linux_usbtmc.h"

MX_RECORD_FUNCTION_LIST mxi_linux_usbtmc_record_function_list = {
	NULL,
	mxi_linux_usbtmc_create_record_structures,
	mx_gpib_finish_record_initialization,
	NULL,
	NULL,
	mxi_linux_usbtmc_open
};

MX_GPIB_FUNCTION_LIST mxi_linux_usbtmc_gpib_function_list = {
	NULL,
	NULL,
	mxi_linux_usbtmc_read,
	mxi_linux_usbtmc_write,
	mxi_linux_usbtmc_interface_clear,
	mxi_linux_usbtmc_device_clear,
	mxi_linux_usbtmc_selective_device_clear,
	mxi_linux_usbtmc_local_lockout,
	mxi_linux_usbtmc_remote_enable,
	mxi_linux_usbtmc_go_to_local,
	mxi_linux_usbtmc_trigger,
	mxi_linux_usbtmc_wait_for_service_request,
	mxi_linux_usbtmc_serial_poll,
	mxi_linux_usbtmc_serial_poll_disable
};

MX_RECORD_FIELD_DEFAULTS mxi_linux_usbtmc_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_GPIB_STANDARD_FIELDS,
	MXI_LINUX_USBTMC_STANDARD_FIELDS
};

long mxi_linux_usbtmc_num_record_fields
		= sizeof( mxi_linux_usbtmc_record_field_defaults )
			/ sizeof( mxi_linux_usbtmc_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_linux_usbtmc_rfield_def_ptr
			= &mxi_linux_usbtmc_record_field_defaults[0];

/* ==== Private function for the driver's use only. ==== */

static mx_status_type
mxi_linux_usbtmc_get_pointers( MX_GPIB *gpib,
				MX_LINUX_USBTMC **linux_usbtmc,
				const char *calling_fname )
{
	static const char fname[] = "mxi_linux_usbtmc_get_pointers()";

	if ( gpib == (MX_GPIB *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_GPIB pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( linux_usbtmc == (MX_LINUX_USBTMC **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_LINUX_USBTMC pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( linux_usbtmc != (MX_LINUX_USBTMC **) NULL ) {
		*linux_usbtmc = (MX_LINUX_USBTMC *)
					gpib->record->record_type_struct;
	}

	if ( *linux_usbtmc == (MX_LINUX_USBTMC *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_LINUX_USBTMC pointer for record '%s' is NULL.",
			gpib->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ==== Public functions ==== */

MX_EXPORT mx_status_type
mxi_linux_usbtmc_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxi_linux_usbtmc_create_record_structures()";

	MX_GPIB *gpib;
	MX_LINUX_USBTMC *linux_usbtmc = NULL;

	/* Allocate memory for the necessary structures. */

	gpib = (MX_GPIB *) malloc( sizeof(MX_GPIB) );

	if ( gpib == (MX_GPIB *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_GPIB structure." );
	}

	linux_usbtmc = (MX_LINUX_USBTMC *) malloc( sizeof(MX_LINUX_USBTMC) );

	if ( linux_usbtmc == (MX_LINUX_USBTMC *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_LINUX_USBTMC structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = gpib;
	record->record_type_struct = linux_usbtmc;
	record->class_specific_function_list
				= &mxi_linux_usbtmc_gpib_function_list;

	gpib->record = record;
	linux_usbtmc->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_linux_usbtmc_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_linux_usbtmc_open()";

	MX_GPIB *gpib;
	MX_LINUX_USBTMC *linux_usbtmc = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	gpib = (MX_GPIB *) record->record_class_struct;

	mx_status = mxi_linux_usbtmc_get_pointers( gpib, &linux_usbtmc, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	gpib->read_buffer_length = 0;
	gpib->write_buffer_length = 0;

	mx_status = mx_gpib_setup_communication_buffers( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	linux_usbtmc->usbtmc_fd = open( linux_usbtmc->filename, O_RDWR );

	if ( linux_usbtmc->usbtmc_fd < 0 ) {
		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Cannot open USBTMC device '%s'.  "
		"errno = %d, error message = '%s'.",
			linux_usbtmc->filename,
			errno, strerror(errno) );
	}

	return mx_status;
}

/* ========== Device specific calls ========== */

MX_EXPORT mx_status_type
mxi_linux_usbtmc_read( MX_GPIB *gpib,
		long address,
		char *buffer,
		size_t max_bytes_to_read,
		size_t *bytes_read,
		unsigned long transfer_flags )
{
	static const char fname[] = "mxi_linux_usbtmc_read()";

	MX_LINUX_USBTMC *linux_usbtmc = NULL;
	int debug;
	size_t local_bytes_read;
	unsigned long read_terminator;
	mx_status_type mx_status;

	mx_status = mxi_linux_usbtmc_get_pointers( gpib, &linux_usbtmc, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_LINUX_USBTMC_DEBUG
	debug = TRUE;
#else
	if ( transfer_flags & MXF_GPIB_DEBUG ) {
		debug = TRUE;

		transfer_flags &= ( ~ MXF_GPIB_DEBUG );
	} else {
		debug = FALSE;
	}
#endif
	read_terminator = gpib->read_terminator[address];

	local_bytes_read = read( linux_usbtmc->usbtmc_fd,
				buffer, max_bytes_to_read );

	if ( local_bytes_read < 0 ) {
		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Error reading from '%s' for record '%s'.  "
		"errno = %d, error message = '%s'.",
			linux_usbtmc->filename,
			gpib->record->name,
			errno, strerror( errno ) );
	}

	/* If present, delete the read terminator character at the end
	 * of the string.
	 */

	if ( buffer[local_bytes_read - 1] == read_terminator ) {
		buffer[ local_bytes_read - 1 ] = '\0';

		local_bytes_read--;
	}

	if ( bytes_read != (size_t *) NULL ) {
		*bytes_read = local_bytes_read;
	}

	if ( debug ) {
		MX_DEBUG(-2,("%s: received '%s' from '%s'.",
				fname, buffer, linux_usbtmc->record->name ));
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_linux_usbtmc_write( MX_GPIB *gpib,
		long address,
		char *buffer,
		size_t bytes_to_write,
		size_t *bytes_written,
		unsigned long transfer_flags )
{
	static const char fname[] = "mxi_linux_usbtmc_write()";

	MX_LINUX_USBTMC *linux_usbtmc = NULL;
	int debug;
	size_t local_bytes_written;
	mx_status_type mx_status;

	mx_status = mxi_linux_usbtmc_get_pointers( gpib, &linux_usbtmc, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_LINUX_USBTMC_DEBUG
	debug = TRUE;
#else
	if ( transfer_flags & MXF_GPIB_DEBUG ) {
		debug = TRUE;

		transfer_flags &= ( ~ MXF_GPIB_DEBUG );
	} else {
		debug = FALSE;
	}
#endif

	if ( debug ) {
		MX_DEBUG(-2,("%s: sending '%s' to '%s'.",
			fname, buffer, linux_usbtmc->record->name ));
	}

	local_bytes_written = write( linux_usbtmc->usbtmc_fd,
					buffer, bytes_to_write );

	if ( local_bytes_written < 0 ) {
		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Error writing string '%s' to '%s' for record '%s'.  "
		"errno = %d, error message = '%s'.",
			buffer, linux_usbtmc->filename,
			gpib->record->name,
			errno, strerror( errno ) );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_linux_usbtmc_interface_clear( MX_GPIB *gpib )
{
	static const char fname[] = "mxi_linux_usbtmc_interface_clear()";

	MX_LINUX_USBTMC *linux_usbtmc = NULL;
	mx_status_type mx_status;

	mx_status = mxi_linux_usbtmc_get_pointers( gpib, &linux_usbtmc, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_linux_usbtmc_device_clear( MX_GPIB *gpib )
{
	int i;

	/* Simulate a device clear by doing a selective device clear
	 * to all devices on the GPIB bus.
	 */

	for ( i = 0; i <= 30; i++ ) {
		(void) mxi_linux_usbtmc_selective_device_clear( gpib, i );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_linux_usbtmc_selective_device_clear( MX_GPIB *gpib, long address )
{
	static const char fname[] = "mxi_linux_usbtmc_selective_device_clear()";

	MX_LINUX_USBTMC *linux_usbtmc = NULL;
	mx_status_type mx_status;

	mx_status = mxi_linux_usbtmc_get_pointers( gpib, &linux_usbtmc, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_linux_usbtmc_local_lockout( MX_GPIB *gpib )
{
	static const char fname[] = "mxi_linux_usbtmc_local_lockout()";

	return mx_error( MXE_UNSUPPORTED, fname,
	"Local lockout is not supported by GPIB interface '%s'.",
		gpib->record->name );
}

MX_EXPORT mx_status_type
mxi_linux_usbtmc_remote_enable( MX_GPIB *gpib, long address )
{
	static const char fname[] = "mxi_linux_usbtmc_remote_enable()";

	return mx_error( MXE_UNSUPPORTED, fname,
	"Remote enable is not supported by GPIB interface '%s'.",
		gpib->record->name );
}

MX_EXPORT mx_status_type
mxi_linux_usbtmc_go_to_local( MX_GPIB *gpib, long address )
{
	static const char fname[] = "mxi_linux_usbtmc_go_to_local()";

	MX_LINUX_USBTMC *linux_usbtmc = NULL;
	mx_status_type mx_status;

	mx_status = mxi_linux_usbtmc_get_pointers( gpib, &linux_usbtmc, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_linux_usbtmc_trigger( MX_GPIB *gpib, long address )
{
	static const char fname[] = "mxi_linux_usbtmc_trigger_device()";

	MX_LINUX_USBTMC *linux_usbtmc = NULL;
	mx_status_type mx_status;

	mx_status = mxi_linux_usbtmc_get_pointers( gpib, &linux_usbtmc, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_linux_usbtmc_wait_for_service_request( MX_GPIB *gpib, double timeout )
{
	static const char fname[] =
			"mxi_linux_usbtmc_wait_for_service_request()";

	MX_LINUX_USBTMC *linux_usbtmc = NULL;
	mx_status_type mx_status;

	mx_status = mxi_linux_usbtmc_get_pointers( gpib, &linux_usbtmc, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return mx_error( MXE_TIMED_OUT, fname,
		"Timed out after waiting %g seconds for the SRQ line "
		"to be asserted by GPIB interface '%s'.",
			timeout, gpib->record->name );
}

MX_EXPORT mx_status_type
mxi_linux_usbtmc_serial_poll( MX_GPIB *gpib, long address,
				unsigned char *serial_poll_byte)
{
	static const char fname[] = "mxi_linux_usbtmc_serial_poll()";

	MX_LINUX_USBTMC *linux_usbtmc = NULL;
	mx_status_type mx_status;

	mx_status = mxi_linux_usbtmc_get_pointers( gpib, &linux_usbtmc, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the serial poll byte. */

	*serial_poll_byte = 0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_linux_usbtmc_serial_poll_disable( MX_GPIB *gpib )
{
	return MX_SUCCESSFUL_RESULT;
}

#endif /* OS_LINUX */

