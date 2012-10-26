/*
 * Name:    i_linux_portio.c
 *
 * Purpose: MX driver for the Linux 'portio' device driver.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2003, 2006, 2008, 2010, 2012
 *    Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_LINUX_PORTIO_DEBUG_TIMING	FALSE

#include <stdio.h>

#if defined(__BORLANDC__)
#define __signed__ signed
#endif

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <asm/types.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_stdint.h"
#include "mx_portio.h"

#include "i_linux_portio.h"

#include "portio.h"

#if MXI_LINUX_PORTIO_DEBUG_TIMING
#  include "mx_hrt_debug.h"
#endif

MX_RECORD_FUNCTION_LIST mxi_linux_portio_record_function_list = {
	NULL,
	mxi_linux_portio_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxi_linux_portio_open,
	mxi_linux_portio_close
};

MX_PORTIO_FUNCTION_LIST mxi_linux_portio_portio_function_list = {
	mxi_linux_portio_inp8,
	mxi_linux_portio_inp16,
	mxi_linux_portio_outp8,
	mxi_linux_portio_outp16,
	mxi_linux_portio_request_region,
	mxi_linux_portio_release_region
};

MX_RECORD_FIELD_DEFAULTS mxi_linux_portio_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_LINUX_PORTIO_STANDARD_FIELDS
};

long mxi_linux_portio_num_record_fields
		= sizeof( mxi_linux_portio_record_field_defaults )
			/ sizeof( mxi_linux_portio_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_linux_portio_rfield_def_ptr
			= &mxi_linux_portio_record_field_defaults[0];

/* ---- */

MX_EXPORT mx_status_type
mxi_linux_portio_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxi_linux_portio_create_record_structures()";

	MX_LINUX_PORTIO *linux_portio;

	/* Allocate memory for the necessary structures. */

	linux_portio = (MX_LINUX_PORTIO *) malloc( sizeof(MX_LINUX_PORTIO) );

	if ( linux_portio == (MX_LINUX_PORTIO *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_LINUX_PORTIO structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = NULL;
	record->record_type_struct = linux_portio;
	record->class_specific_function_list
				= &mxi_linux_portio_portio_function_list;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_linux_portio_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_linux_portio_open()";

	MX_LINUX_PORTIO *linux_portio;
	int saved_errno;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL.");
	}

	linux_portio = (MX_LINUX_PORTIO *) (record->record_type_struct);

	if ( linux_portio == (MX_LINUX_PORTIO *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_LINUX_PORTIO pointer for record '%s' is NULL.",
			record->name );
	}

	linux_portio->file_handle = open( linux_portio->filename, O_RDWR );

	if ( linux_portio->file_handle < 0 ) {
		saved_errno = errno;

		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"Error opening Linux portio device '%s'.  "
			"Errno = %d, errno text = '%s'",
			record->name, saved_errno, strerror( saved_errno ) );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_linux_portio_close( MX_RECORD *record )
{
	static const char fname[] = "mxi_linux_portio_close()";

	MX_LINUX_PORTIO *linux_portio;
	int result;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	linux_portio = (MX_LINUX_PORTIO *) (record->record_type_struct);

	if ( linux_portio == (MX_LINUX_PORTIO *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_LINUX_PORTIO pointer for record '%s' is NULL.",
			record->name );
	}

	/* If the Linux portio device is currently open, close it. */

	if ( linux_portio->file_handle >= 0 ) {
		result = close( linux_portio->file_handle );

		if ( result != 0 ) {
			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"Error while closing Linux portio device '%s'.",
				record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT uint8_t
mxi_linux_portio_inp8( MX_RECORD *record, unsigned long port_number )
{
	static const char fname[] = "mxi_linux_portio_inp8()";

	MX_LINUX_PORTIO *linux_portio;
	portio_message message;
	int status, saved_errno;

#if MXI_LINUX_PORTIO_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	linux_portio = ( MX_LINUX_PORTIO *) record->record_type_struct;

	message.address = port_number;

#if MXI_LINUX_PORTIO_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	status = ioctl( linux_portio->file_handle,
				PORTIO_INB_IOCTL, &message );

#if MXI_LINUX_PORTIO_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname,
		"read %#x from port %#lx", message.arg.byte, port_number );
#endif

	if ( status < 0 ) {
		saved_errno = errno;

		(void) mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Error reading 8 bit integer from record '%s', port %#lx.  "
		"Error number = %d, error text = '%s'",
			record->name, port_number,
			saved_errno, strerror( saved_errno ) );
	}

	return message.arg.byte;
}

MX_EXPORT uint16_t
mxi_linux_portio_inp16( MX_RECORD *record, unsigned long port_number )
{
	static const char fname[] = "mxi_linux_portio_inp16()";

	MX_LINUX_PORTIO *linux_portio;
	portio_message message;
	int status, saved_errno;

#if MXI_LINUX_PORTIO_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	linux_portio = ( MX_LINUX_PORTIO *) record->record_type_struct;

	message.address = port_number;

#if MXI_LINUX_PORTIO_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	status = ioctl( linux_portio->file_handle,
				PORTIO_INW_IOCTL, &message );

#if MXI_LINUX_PORTIO_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname,
		"read %#x from port %#lx", message.arg.word, port_number );
#endif

	if ( status < 0 ) {
		saved_errno = errno;

		(void) mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Error reading 16 bit integer from record '%s', port %#lx.  "
		"Error number = %d, error text = '%s'",
			record->name, port_number,
			saved_errno, strerror( saved_errno ) );
	}

	return message.arg.word;
}

MX_EXPORT void
mxi_linux_portio_outp8( MX_RECORD *record,
			unsigned long port_number,
			uint8_t byte_value )
{
	static const char fname[] = "mxi_linux_portio_outp8()";

	MX_LINUX_PORTIO *linux_portio;
	portio_message message;
	int status, saved_errno;

#if MXI_LINUX_PORTIO_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	linux_portio = ( MX_LINUX_PORTIO *) record->record_type_struct;

	message.address = port_number;
	message.arg.byte = byte_value;

#if MXI_LINUX_PORTIO_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	status = ioctl( linux_portio->file_handle,
				PORTIO_OUTB_IOCTL, &message );

#if MXI_LINUX_PORTIO_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname,
		"wrote %#x to port %#lx", byte_value, port_number );
#endif

	if ( status < 0 ) {
		saved_errno = errno;

		(void) mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Error writing 8 bit integer %#x to record '%s', port %#lx.  "
		"Error number = %d, error text = '%s'",
			byte_value, record->name, port_number,
			saved_errno, strerror( saved_errno ) );
	}

	return;
}

MX_EXPORT void
mxi_linux_portio_outp16( MX_RECORD *record,
			unsigned long port_number,
			uint16_t word_value )
{
	static const char fname[] = "mxi_linux_portio_outp16()";

	MX_LINUX_PORTIO *linux_portio;
	portio_message message;
	int status, saved_errno;

#if MXI_LINUX_PORTIO_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	linux_portio = ( MX_LINUX_PORTIO *) record->record_type_struct;

	message.address = port_number;
	message.arg.word = word_value;

#if MXI_LINUX_PORTIO_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	status = ioctl( linux_portio->file_handle,
				PORTIO_OUTW_IOCTL, &message );

#if MXI_LINUX_PORTIO_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname,
		"wrote %#x to port %#lx", word_value, port_number );
#endif

	if ( status < 0 ) {
		saved_errno = errno;

		(void) mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Error writing 16 bit integer %#x to record '%s', port %#lx.  "
		"Error number = %d, error text = '%s'",
			word_value, record->name, port_number,
			saved_errno, strerror( saved_errno ) );
	}

	return;
}

MX_EXPORT mx_status_type
mxi_linux_portio_request_region( MX_RECORD *record,
				unsigned long port_number,
				unsigned long length )
{
	static const char fname[] = "mxi_linux_portio_request_region()";

	portio_permission_map map;
	MX_LINUX_PORTIO *linux_portio;
	int i, starting_index, status, access_allowed, saved_errno;

	linux_portio = ( MX_LINUX_PORTIO *) record->record_type_struct;

	status = ioctl( linux_portio->file_handle,
				PORTIO_GETMAP_IOCTL, &map );

	if ( status < 0 ) {
		saved_errno = errno;

		(void) mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Error reading permission map for record '%s'.  "
		"Error number = %d, error text = '%s'",
		record->name, saved_errno, strerror( saved_errno ) );
	}

	access_allowed = 1;

	starting_index = port_number - PORTIO_MIN_ADDRESS;

	for ( i = starting_index; i < length; i++ ) {

		if ( map.map[i] == 0 ) {
			access_allowed = 0;
			break;
		}
	}

	if ( access_allowed == 0 ) {

		return mx_error( MXE_PERMISSION_DENIED, fname,
		"Access to I/O port range %#lx-%#lx is not permitted.",
			port_number, port_number + length - 1 );
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_linux_portio_release_region( MX_RECORD *record,
				unsigned long port_number,
				unsigned long length )
{
	return MX_SUCCESSFUL_RESULT;
}

