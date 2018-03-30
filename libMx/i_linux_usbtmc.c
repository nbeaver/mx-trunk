/*
 * Name:    i_linux_usbtmc.c
 *
 * Purpose: Linux MX GPIB driver for USBTMC devices controlled via
 *          /dev/usbtmc0 and friends.
 *
 * Note:    This driver only supports one "GPIB device" at a time.
 *          Because of this, it completely ignores the GPIB address
 *          supplied by the caller.
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
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <linux/usb/tmc.h>

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

	/* Create a file descriptor to talk to the Linux 'usbtmc' module. */

	linux_usbtmc->usbtmc_fd = open( linux_usbtmc->filename, O_RDWR );

	if ( linux_usbtmc->usbtmc_fd < 0 ) {
		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Cannot open USBTMC device '%s'.  "
		"errno = %d, error message = '%s'.",
			linux_usbtmc->filename,
			errno, strerror(errno) );
	}

	/* Get a list of the available USB488 capabilities, if available. */

#if defined( USBTMC488_IOCTL_GET_CAPS )

	{
		int ioctl_status, int_capabilities;

		ioctl_status = ioctl( linux_usbtmc->usbtmc_fd,
					USBTMC488_IOCTL_GET_CAPS,
					&int_capabilities );

		linux_usbtmc->usb488_capabilities
				= (unsigned long) int_capabilities;
	}
#else
	linux_usbtmc->usb488_capabilities = 0;
#endif

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

	/* Null terminate the string. */

	buffer[local_bytes_read] = '\0';

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
	mx_status_type mx_status;

	mx_status = mxi_linux_usbtmc_device_clear( gpib );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_linux_usbtmc_device_clear( MX_GPIB *gpib )
{
	static const char fname[] = "mxi_linux_usbtmc_device_clear()";

	MX_LINUX_USBTMC *linux_usbtmc = NULL;
	int ioctl_status;
	mx_status_type mx_status;

	mx_status = mxi_linux_usbtmc_get_pointers( gpib, &linux_usbtmc, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ioctl_status = ioctl( linux_usbtmc->usbtmc_fd,
				USBTMC_IOCTL_CLEAR );

	MX_DEBUG(-2,("%s: ioctl_status = %d", fname, ioctl_status));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_linux_usbtmc_selective_device_clear( MX_GPIB *gpib, long address )
{
	mx_status_type mx_status;

	mx_status = mxi_linux_usbtmc_device_clear( gpib );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_linux_usbtmc_local_lockout( MX_GPIB *gpib )
{
	static const char fname[] = "mxi_linux_usbtmc_local_lockout()";

#if !defined( USBTMC488_CAPABILITY_LOCAL_LOCKOUT )
	return mx_error( MXE_UNSUPPORTED, fname,
	"Local lockout is not implemented for this version of Linux." );
#else
	MX_LINUX_USBTMC *linux_usbtmc = NULL;
	int ioctl_status;
	unsigned long available;
	mx_status_type mx_status;

	mx_status = mxi_linux_usbtmc_get_pointers( gpib, &linux_usbtmc, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	available =
    ( linux_usbtmc->usb488_capabilities & USBTMC488_CAPABILITY_LOCAL_LOCKOUT );

	if ( available ) {
		ioctl_status = ioctl( linux_usbtmc->usbtmc_fd,
				USBTMC488_IOCTL_LOCAL_LOCKOUT );

		MX_DEBUG(-2,("%s: ioctl_status = %d", fname, ioctl_status));
	}

	return MX_SUCCESSFUL_RESULT;
#endif
}

MX_EXPORT mx_status_type
mxi_linux_usbtmc_remote_enable( MX_GPIB *gpib, long address )
{
	static const char fname[] = "mxi_linux_usbtmc_remote_enable()";

#if !defined( USBTMC488_CAPABILITY_REN_CONTROL )
	return mx_error( MXE_UNSUPPORTED, fname,
	"Remote enable is not implemented for this version of Linux." );
#else
	MX_LINUX_USBTMC *linux_usbtmc = NULL;
	int ioctl_status;
	unsigned long available;
	mx_status_type mx_status;

	mx_status = mxi_linux_usbtmc_get_pointers( gpib, &linux_usbtmc, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	available =
    ( linux_usbtmc->usb488_capabilities & USBTMC488_CAPABILITY_REN_CONTROL );

	if ( available ) {
		ioctl_status = ioctl( linux_usbtmc->usbtmc_fd,
				USBTMC488_IOCTL_REN_CONTROL );

		MX_DEBUG(-2,("%s: ioctl_status = %d", fname, ioctl_status));
	}

	return MX_SUCCESSFUL_RESULT;
#endif
}

MX_EXPORT mx_status_type
mxi_linux_usbtmc_go_to_local( MX_GPIB *gpib, long address )
{
	static const char fname[] = "mxi_linux_usbtmc_go_to_local()";

#if !defined( USBTMC488_CAPABILITY_GOTO_LOCAL )
	return mx_error( MXE_UNSUPPORTED, fname,
		"Go to local is not implemented for this version of Linux." );
#else
	MX_LINUX_USBTMC *linux_usbtmc = NULL;
	int ioctl_status;
	unsigned long available;
	mx_status_type mx_status;

	mx_status = mxi_linux_usbtmc_get_pointers( gpib, &linux_usbtmc, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	available =
    ( linux_usbtmc->usb488_capabilities & USBTMC488_CAPABILITY_GOTO_LOCAL );

	if ( available ) {
		ioctl_status = ioctl( linux_usbtmc->usbtmc_fd,
				USBTMC488_IOCTL_GOTO_LOCAL );

		MX_DEBUG(-2,("%s: ioctl_status = %d", fname, ioctl_status));
	}

	return MX_SUCCESSFUL_RESULT;
#endif
}

MX_EXPORT mx_status_type
mxi_linux_usbtmc_trigger( MX_GPIB *gpib, long address )
{
	static const char fname[] = "mxi_linux_usbtmc_trigger_device()";

#if !defined( USBTMC488_CAPABILITY_TRIGGER )
	return mx_error( MXE_UNSUPPORTED, fname,
	"Trigger is not implemented for this version of Linux." );
#else
	MX_LINUX_USBTMC *linux_usbtmc = NULL;
	unsigned long available;
	mx_status_type mx_status;

	mx_status = mxi_linux_usbtmc_get_pointers( gpib, &linux_usbtmc, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	available =
    ( linux_usbtmc->usb488_capabilities & USBTMC488_CAPABILITY_TRIGGER );

	if ( available ) {
		int ioctl_status;

		ioctl_status = ioctl( linux_usbtmc->usbtmc_fd,
				USBTMC488_IOCTL_TRIGGER );

		MX_DEBUG(-2,("%s: ioctl_status = %d", fname, ioctl_status));
	}

	return MX_SUCCESSFUL_RESULT;
#endif
}

MX_EXPORT mx_status_type
mxi_linux_usbtmc_wait_for_service_request( MX_GPIB *gpib, double timeout )
{
	static const char fname[] =
			"mxi_linux_usbtmc_wait_for_service_request()";

	MX_LINUX_USBTMC *linux_usbtmc = NULL;
	struct pollfd poll_struct;
	int poll_status, timeout_ms;
	mx_status_type mx_status;

	mx_status = mxi_linux_usbtmc_get_pointers( gpib, &linux_usbtmc, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	timeout_ms = mx_round( 1000.0 * timeout );

	poll_struct.fd = linux_usbtmc->usbtmc_fd;
	poll_struct.events = POLLIN;

	poll_status = poll( &poll_struct, 1, timeout_ms );

	if ( poll_status < 0 ) {
		return mx_error( MXE_FILE_IO_ERROR, fname,
		"The attempt to wait for a service request for '%s' failed.  "
		"errno = %d, error message = '%s'.",
			gpib->record->name,
			errno, strerror( errno ) );
	}

	/* FIXME: Should we do something more elaborate with the results
	 * from poll()?
	 */

	return MX_SUCCESSFUL_RESULT;
}


MX_EXPORT mx_status_type
mxi_linux_usbtmc_serial_poll( MX_GPIB *gpib, long address,
				unsigned char *serial_poll_byte )
{
	static const char fname[] = "mxi_linux_usbtmc_serial_poll()";

#if !defined( USBTMC488_IOCTL_READ_STB )
	return mx_error( MXE_UNSUPPORTED, fname,
	"Serial poll is not implemented for this version of Linux." );
#else
	MX_LINUX_USBTMC *linux_usbtmc = NULL;
	int ioctl_status;
	mx_status_type mx_status;

	mx_status = mxi_linux_usbtmc_get_pointers( gpib, &linux_usbtmc, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the serial poll byte. */

	ioctl_status = ioctl( linux_usbtmc->usbtmc_fd,
				USBTMC488_IOCTL_READ_STB,
				serial_poll_byte );

	MX_DEBUG(-2,("%s: ioctl_status = %d", fname, ioctl_status));
	MX_DEBUG(-2,("%s: serial_poll_byte = %#x",
			fname, (unsigned int) (*serial_poll_byte) ));

	return MX_SUCCESSFUL_RESULT;
#endif
}

MX_EXPORT mx_status_type
mxi_linux_usbtmc_serial_poll_disable( MX_GPIB *gpib )
{
	return MX_SUCCESSFUL_RESULT;
}

#endif /* OS_LINUX */

