/*
 * Name:    mx_gpib.c
 *
 * Purpose: MX function library of generic GPIB operations.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2004-2006, 2018 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_gpib.h"
#include "mx_driver.h"

MX_EXPORT mx_status_type
mx_gpib_finish_record_initialization( MX_RECORD *gpib_record )
{
	static const char fname[] = "mx_gpib_finish_record_initialization()";

	MX_GPIB *gpib;
	int i;

	if ( gpib_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	gpib = (MX_GPIB *) gpib_record->record_class_struct;

	if ( gpib == (MX_GPIB *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_GPIB pointer for record '%s' is NULL.",
			gpib_record->name );
	}

	/* Initialize the field values for all of the GPIB addresses. */


	for ( i = 0; i < MX_NUM_GPIB_ADDRESSES; i++ ) {

		gpib->open_semaphore[i] = 0;
		gpib->secondary_address[i] = 0;

		gpib->io_timeout[i] = gpib->default_io_timeout;
		gpib->eoi_mode[i] = gpib->default_eoi_mode;
		gpib->eos_mode[i] = gpib->default_eos_mode;
		gpib->read_terminator[i] = gpib->default_read_terminator;
		gpib->write_terminator[i] = gpib->default_write_terminator;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_gpib_setup_communication_buffers( MX_RECORD *gpib_record )
{
	static const char fname[] = "mx_gpib_setup_communication_buffers()";

	MX_GPIB *gpib;
	MX_LIST_HEAD *list_head;
	MX_RECORD_FIELD *field;
	long saved_read_buffer_length, saved_write_buffer_length;
	mx_status_type mx_status;

	if ( gpib_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	gpib = (MX_GPIB *) gpib_record->record_class_struct;

	if ( gpib == (MX_GPIB *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_GPIB pointer for record '%s' is NULL.",
			gpib_record->name );
	}

	list_head = mx_get_record_list_head_struct( gpib_record );

	if ( list_head == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_LIST_HEAD pointer for record '%s' is NULL.",
			gpib_record->name );
	}

	/* If we are running in a server, allocate buffers for read
	 * and write operations.
	 */

	if ( list_head->is_server == FALSE ) {
		gpib->read_buffer_length = 0;
		gpib->write_buffer_length = 0;
		gpib->read_buffer = NULL;
		gpib->write_buffer = NULL;
	} else {
		if ( gpib->read_buffer_length <= 0 ) {
			gpib->read_buffer_length
				= MX_GPIB_DEFAULT_BUFFER_LENGTH;
		}
		if ( gpib->write_buffer_length <= 0 ) {
			gpib->write_buffer_length
				= MX_GPIB_DEFAULT_BUFFER_LENGTH;
		}

		gpib->read_buffer = (char *)
			malloc( gpib->read_buffer_length );

		if ( gpib->read_buffer == NULL ) {
			saved_read_buffer_length = gpib->read_buffer_length;

			gpib->read_buffer_length = 0;
			gpib->write_buffer_length = 0;

			return mx_error( MXE_OUT_OF_MEMORY, fname,
	    "Cannot allocate a %ld byte read buffer for GPIB interface '%s'.",
	    			saved_read_buffer_length, gpib_record->name );
		}

		gpib->write_buffer = (char *)
			malloc( gpib->write_buffer_length );

		if ( gpib->write_buffer == NULL ) {
			saved_write_buffer_length = gpib->write_buffer_length;

			gpib->write_buffer_length = 0;

			return mx_error( MXE_OUT_OF_MEMORY, fname,
	    "Cannot allocate a %ld byte write buffer for GPIB interface '%s'.",
	    			saved_write_buffer_length, gpib_record->name );
		}
	}

	/* Update the 'read', 'write', 'getline', and 'putline' field 
	 * string lengths.
	 */

	mx_status = mx_find_record_field( gpib_record, "read", &field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	field->dimension[0] = gpib->read_buffer_length;

	mx_status = mx_find_record_field( gpib_record, "write", &field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	field->dimension[0] = gpib->write_buffer_length;

	mx_status = mx_find_record_field( gpib_record, "getline", &field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	field->dimension[0] = gpib->read_buffer_length;

	mx_status = mx_find_record_field( gpib_record, "putline", &field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	field->dimension[0] = gpib->write_buffer_length;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_gpib_get_pointers( MX_RECORD *gpib_record,
			MX_GPIB **gpib,
			MX_GPIB_FUNCTION_LIST **function_list_ptr,
			const char *calling_fname )
{
	static const char fname[] = "mx_gpib_get_pointers()";

	if ( gpib_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The gpib_record pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( gpib_record->mx_class != MXI_GPIB ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
	"The record '%s' passed by '%s' is not a GPIB interface.  "
	"(superclass = %ld, class = %ld, type = %ld)",
			gpib_record->name, calling_fname,
			gpib_record->mx_superclass,
			gpib_record->mx_class,
			gpib_record->mx_type );
	}

	if ( gpib != (MX_GPIB **) NULL ) {
		*gpib = (MX_GPIB *) (gpib_record->record_class_struct);

		if ( *gpib == (MX_GPIB *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_GPIB pointer for record '%s' passed by '%s' is NULL.",
				gpib_record->name, calling_fname );
		}
	}

	if ( function_list_ptr != (MX_GPIB_FUNCTION_LIST **) NULL ) {
		*function_list_ptr = (MX_GPIB_FUNCTION_LIST *)
				(gpib_record->class_specific_function_list);

		if ( *function_list_ptr == (MX_GPIB_FUNCTION_LIST *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_GPIB_FUNCTION_LIST pointer for record '%s' passed by '%s' is NULL.",
				gpib_record->name, calling_fname );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_gpib_open_device( MX_RECORD *gpib_record, long address )
{
	static const char fname[] = "mx_gpib_open_device()";

	MX_GPIB *gpib;
	MX_GPIB_FUNCTION_LIST *fl_ptr;
	mx_status_type (*fptr)( MX_GPIB *, long );
	mx_status_type mx_status;

	mx_status = mx_gpib_get_pointers( gpib_record,
					&gpib, &fl_ptr, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( gpib->open_semaphore[ address ] > 0 ) {

		( gpib->open_semaphore[ address ] )++;
	} else {

		fptr = fl_ptr->open_device;

		if ( fptr != NULL ) {
			mx_status = (*fptr)( gpib, address );
		}

		if ( mx_status.code != MXE_SUCCESS ) {
			gpib->open_semaphore[ address ] = 0;

			return mx_status;
		} else {
			gpib->open_semaphore[ address ] = 1;
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_gpib_close_device( MX_RECORD *gpib_record, long address )
{
	static const char fname[] = "mx_gpib_close_device()";

	MX_GPIB *gpib;
	MX_GPIB_FUNCTION_LIST *fl_ptr;
	mx_status_type (*fptr)( MX_GPIB *, long );
	mx_status_type mx_status;

	mx_status = mx_gpib_get_pointers( gpib_record,
					&gpib, &fl_ptr, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( gpib->open_semaphore[ address ] > 1 ) {

		( gpib->open_semaphore[ address ] )--;

	} else if ( gpib->open_semaphore[ address ] <= 0 ) {

		return mx_error( MXE_FUNCTION_FAILED, fname,
		"Attempted to close GPIB address %ld on interface '%s' "
		"when it was already closed.",
			address, gpib_record->name );
	} else {

		fptr = fl_ptr->close_device;

		if ( fptr != NULL ) {
			mx_status = (*fptr)( gpib, address );
		}

		gpib->open_semaphore[ address ] = 0;

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_gpib_read( MX_RECORD *gpib_record,
		long address,
		char *buffer,
		size_t max_bytes_to_read,
		size_t *bytes_read,
		unsigned long flags)
{
	static const char fname[] = "mx_gpib_read()";

	MX_GPIB *gpib;
	MX_GPIB_FUNCTION_LIST *fl_ptr;
	size_t local_bytes_read;
	mx_status_type (*fptr)(MX_GPIB *,
				long, char *, size_t, size_t *, unsigned long);
	mx_status_type mx_status;

	mx_status = mx_gpib_get_pointers( gpib_record,
					&gpib, &fl_ptr, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fptr = fl_ptr->read;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"read function pointer is NULL.");
	}

	gpib->ascii_read = FALSE;

	mx_status = (*fptr)( gpib, address,
			buffer, max_bytes_to_read, &local_bytes_read, flags );

	gpib->bytes_read = (long) local_bytes_read;

	if ( bytes_read != NULL ) {
		*bytes_read = local_bytes_read;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_gpib_write( MX_RECORD *gpib_record,
		long address,
		char *buffer,
		size_t bytes_to_write,
		size_t *bytes_written,
		unsigned long flags)
{
	static const char fname[] = "mx_gpib_write()";

	MX_GPIB *gpib;
	MX_GPIB_FUNCTION_LIST *fl_ptr;
	size_t local_bytes_written;
	mx_status_type (*fptr)(MX_GPIB *,
				long, char *, size_t, size_t *, unsigned long);
	mx_status_type mx_status;

	mx_status = mx_gpib_get_pointers( gpib_record,
					&gpib, &fl_ptr, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fptr = fl_ptr->write;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"write function pointer is NULL.");
	}

	gpib->ascii_write = FALSE;

	mx_status = (*fptr)( gpib, address,
			buffer, bytes_to_write, &local_bytes_written, flags );

	gpib->bytes_written = (long) local_bytes_written;

	if ( bytes_written != NULL ) {
		*bytes_written = local_bytes_written;
	}

	return mx_status;
}

/*
 * mx_gpib_getline() and mx_gpib_putline() are variants of mx_gpib_read()
 * and mx_gpib_write() that are intended for use in sending and receiving
 * ASCII character strings rather than binary data.
 */

MX_EXPORT mx_status_type
mx_gpib_getline( MX_RECORD *gpib_record,
		long address,
		char *buffer,
		size_t max_bytes_to_read,
		size_t *bytes_read,
		unsigned long flags)
{
	static const char fname[] = "mx_gpib_getline()";

	MX_GPIB *gpib;
	MX_GPIB_FUNCTION_LIST *fl_ptr;
	size_t real_max_bytes_to_read, real_bytes_read;
	mx_status_type (*fptr)(MX_GPIB *,
				long, char *, size_t, size_t *, unsigned long);
	mx_status_type mx_status;

	mx_status = mx_gpib_get_pointers( gpib_record,
					&gpib, &fl_ptr, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fptr = fl_ptr->read;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"read function pointer is NULL.");
	}

	gpib->ascii_read = TRUE;

	/* Make sure to leave an extra byte at the end for the '\0'
	 * character to be placed at the end of the string.
	 */

	real_max_bytes_to_read = max_bytes_to_read - 1;

	mx_status = (*fptr)( gpib, address,
		buffer, real_max_bytes_to_read, &real_bytes_read, flags );

	buffer[real_bytes_read] = '\0';

	gpib->bytes_read = (long) ( real_bytes_read + 1 );

	if ( bytes_read != NULL ) {
		*bytes_read = real_bytes_read + 1;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_gpib_putline( MX_RECORD *gpib_record,
		long address,
		char *buffer,
		size_t *bytes_written,
		unsigned long flags)
{
	static const char fname[] = "mx_gpib_putline()";

	MX_GPIB *gpib;
	MX_GPIB_FUNCTION_LIST *fl_ptr;
	size_t bytes_to_write;
	size_t local_bytes_written;
	mx_status_type (*fptr)(MX_GPIB *,
				long, char *, size_t, size_t *, unsigned long);
	mx_status_type mx_status;

	mx_status = mx_gpib_get_pointers( gpib_record,
					&gpib, &fl_ptr, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fptr = fl_ptr->write;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"write function pointer is NULL.");
	}

	gpib->ascii_write = TRUE;

	bytes_to_write = strlen( buffer ) + 1;

	mx_status = (*fptr)( gpib, address,
			buffer, bytes_to_write, &local_bytes_written, flags );

	gpib->bytes_written = (long) local_bytes_written;

	if ( bytes_written != NULL ) {
		*bytes_written = local_bytes_written;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_gpib_interface_clear( MX_RECORD *gpib_record )
{
	static const char fname[] = "mx_gpib_interface_clear()";

	MX_GPIB *gpib;
	MX_GPIB_FUNCTION_LIST *fl_ptr;
	mx_status_type (*fptr)( MX_GPIB * );
	mx_status_type mx_status;

	mx_status = mx_gpib_get_pointers( gpib_record,
					&gpib, &fl_ptr, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fptr = fl_ptr->interface_clear;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"interface_clear function pointer is NULL.");
	}

	mx_status = (*fptr)( gpib );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_gpib_device_clear( MX_RECORD *gpib_record )
{
	static const char fname[] = "mx_gpib_device_clear()";

	MX_GPIB *gpib;
	MX_GPIB_FUNCTION_LIST *fl_ptr;
	mx_status_type (*fptr)( MX_GPIB * );
	mx_status_type mx_status;

	mx_status = mx_gpib_get_pointers( gpib_record,
					&gpib, &fl_ptr, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fptr = fl_ptr->device_clear;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"device_clear function pointer is NULL.");
	}

	mx_status = (*fptr)( gpib );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_gpib_selective_device_clear( MX_RECORD *gpib_record, long address )
{
	static const char fname[] = "mx_gpib_selective_device_clear()";

	MX_GPIB *gpib;
	MX_GPIB_FUNCTION_LIST *fl_ptr;
	mx_status_type (*fptr)( MX_GPIB *, long );
	mx_status_type mx_status;

	mx_status = mx_gpib_get_pointers( gpib_record,
					&gpib, &fl_ptr, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fptr = fl_ptr->selective_device_clear;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"selective_device_clear function pointer is NULL.");
	}

	mx_status = (*fptr)( gpib, address );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_gpib_local_lockout( MX_RECORD *gpib_record )
{
	static const char fname[] = "mx_gpib_local_lockout()";

	MX_GPIB *gpib;
	MX_GPIB_FUNCTION_LIST *fl_ptr;
	mx_status_type (*fptr)( MX_GPIB * );
	mx_status_type mx_status;

	mx_status = mx_gpib_get_pointers( gpib_record,
					&gpib, &fl_ptr, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fptr = fl_ptr->local_lockout;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"local_lockout function pointer is NULL.");
	}

	mx_status = (*fptr)( gpib );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_gpib_remote_enable( MX_RECORD *gpib_record, long address )
{
	static const char fname[] = "mx_gpib_remote_enable()";

	MX_GPIB *gpib;
	MX_GPIB_FUNCTION_LIST *fl_ptr;
	mx_status_type (*fptr)( MX_GPIB *, long );
	mx_status_type mx_status;

	mx_status = mx_gpib_get_pointers( gpib_record,
					&gpib, &fl_ptr, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fptr = fl_ptr->remote_enable;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"remote_enable function pointer is NULL.");
	}

	mx_status = (*fptr)( gpib, address );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_gpib_go_to_local( MX_RECORD *gpib_record, long address )
{
	static const char fname[] = "mx_gpib_go_to_local()";

	MX_GPIB *gpib;
	MX_GPIB_FUNCTION_LIST *fl_ptr;
	mx_status_type (*fptr)( MX_GPIB *, long );
	mx_status_type mx_status;

	mx_status = mx_gpib_get_pointers( gpib_record,
					&gpib, &fl_ptr, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fptr = fl_ptr->go_to_local;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"go_to_local function pointer is NULL.");
	}

	mx_status = (*fptr)( gpib, address );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_gpib_trigger( MX_RECORD *gpib_record, long address )
{
	static const char fname[] = "mx_gpib_trigger()";

	MX_GPIB *gpib;
	MX_GPIB_FUNCTION_LIST *fl_ptr;
	mx_status_type (*fptr)( MX_GPIB *, long );
	mx_status_type mx_status;

	mx_status = mx_gpib_get_pointers( gpib_record,
					&gpib, &fl_ptr, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fptr = fl_ptr->trigger;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"trigger function pointer is NULL.");
	}

	mx_status = (*fptr)( gpib, address );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_gpib_wait_for_service_request( MX_RECORD *gpib_record, double timeout )
{
	static const char fname[] = "mx_gpib_wait_for_service_request()";

	MX_GPIB *gpib;
	MX_GPIB_FUNCTION_LIST *fl_ptr;
	mx_status_type (*fptr)( MX_GPIB *, double );
	mx_status_type mx_status;

	mx_status = mx_gpib_get_pointers( gpib_record,
					&gpib, &fl_ptr, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fptr = fl_ptr->wait_for_service_request;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"wait_for_service_request function pointer is NULL.");
	}

	mx_status = (*fptr)( gpib, timeout );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_gpib_serial_poll( MX_RECORD *gpib_record,
			long address, unsigned char *serial_poll_byte )
{
	static const char fname[] = "mx_gpib_serial_poll()";

	MX_GPIB *gpib;
	MX_GPIB_FUNCTION_LIST *fl_ptr;
	mx_status_type (*fptr)( MX_GPIB *, long, unsigned char * );
	mx_status_type mx_status;

	mx_status = mx_gpib_get_pointers( gpib_record,
					&gpib, &fl_ptr, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fptr = fl_ptr->serial_poll;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"serial_poll function pointer is NULL.");
	}

	mx_status = (*fptr)( gpib, address, serial_poll_byte );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_gpib_serial_poll_disable( MX_RECORD *gpib_record )
{
	static const char fname[] = "mx_gpib_serial_poll_disable()";

	MX_GPIB *gpib;
	MX_GPIB_FUNCTION_LIST *fl_ptr;
	mx_status_type (*fptr)( MX_GPIB * );
	mx_status_type mx_status;

	mx_status = mx_gpib_get_pointers( gpib_record,
					&gpib, &fl_ptr, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fptr = fl_ptr->serial_poll_disable;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"serial_poll_disable function pointer is NULL.");
	}

	mx_status = (*fptr)( gpib );

	return mx_status;
}

