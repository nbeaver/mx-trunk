/*
 * Name:    mx_circular_buffer.c
 *
 * Purpose: Support for circular or ring buffers.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2012-2013 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_mutex.h"
#include "mx_circular_buffer.h"

MX_EXPORT mx_status_type
mx_circular_buffer_create( MX_CIRCULAR_BUFFER **buffer,
			unsigned long buffer_size )
{
	static const char fname[] = "mx_circular_buffer_create()";

	mx_status_type mx_status;

	if ( buffer == (MX_CIRCULAR_BUFFER **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_CIRCULAR_BUFFER pointer passed was NULL." );
	}
	if ( buffer_size == 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Zero length circular buffers are not supported." );
	}

	/* Allocate the memory for the MX_CIRCULAR_BUFFER structure. */

	*buffer = (MX_CIRCULAR_BUFFER *) malloc( sizeof(MX_CIRCULAR_BUFFER) );

	if ( (*buffer) == (MX_CIRCULAR_BUFFER *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate an "
		"MX_CIRCULAR_BUFFER structure." );
	}

	(*buffer)->buffer_size   = buffer_size;
	(*buffer)->bytes_written = 0;
	(*buffer)->bytes_read    = 0;

	/* Allocate the data buffer. */

	(*buffer)->data_array = (char *) malloc( buffer_size );

	if ( (*buffer)->data_array == (char *) NULL ) {
		mx_free( (*buffer) );

		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %lu byte "
		"data array for an MX_CIRCULAR_BUFFER.",
			buffer_size );
	}

	/* Create a mutex to control access to the buffer. */

	mx_status = mx_mutex_create( &((*buffer)->mutex) );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_free( (*buffer)->data_array );
		mx_free( (*buffer) );

		return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_circular_buffer_destroy( MX_CIRCULAR_BUFFER *buffer )
{
	static const char fname[] = "mx_circular_buffer_destroy()";

	mx_status_type mx_status;

	if ( buffer == (MX_CIRCULAR_BUFFER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_CIRCULAR_BUFFER pointer passed was NULL." );
	}

	/* First, we destroy the mutex. */

	mx_status = mx_mutex_destroy( buffer->mutex );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Then, we free the allocated memory. */

	mx_free( buffer->data_array );
	mx_free( buffer );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_circular_buffer_read( MX_CIRCULAR_BUFFER *buffer,
			char *data_destination,
			unsigned long max_bytes_to_read,
			unsigned long *num_bytes_read )
{
	static const char fname[] = "mx_circular_buffer_read()";

	unsigned long num_bytes_peeked;
	long mx_status_code;
	mx_status_type mx_status;

	/* Lock the mutex. */

	mx_status_code = mx_mutex_lock( buffer->mutex );

	if ( mx_status_code != MXE_SUCCESS ) {
		return mx_error( mx_status_code, fname,
		"The attempt to lock MX_CIRCULAR_BUFFER %p failed.",
			buffer );
	}

	/* Copy the data from the buffer. */

	mx_status = mx_circular_buffer_peek( buffer,
					data_destination,
					max_bytes_to_read,
					&num_bytes_peeked );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Mark the data as read. */

	mx_status = mx_circular_buffer_increment_read_bytes( buffer,
							num_bytes_peeked );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Unlock the mutex. */

	mx_status_code = mx_mutex_unlock( buffer->mutex );

	if ( mx_status_code != MXE_SUCCESS ) {
		return mx_error( mx_status_code, fname,
		"The attempt to unlock MX_CIRCULAR_BUFFER %p failed.",
			buffer );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_circular_buffer_peek( MX_CIRCULAR_BUFFER *buffer,
			char *data_destination,
			unsigned long max_bytes_to_peek,
			unsigned long *num_bytes_peeked )
{
	static const char fname[] = "mx_circular_buffer_peek()";

	unsigned long num_bytes_in_use, num_bytes_to_peek;
	unsigned long bytes_read_modulo;
	unsigned long future_value_of_bytes_read;
	unsigned long future_value_of_bytes_read_modulo;
	unsigned long bytes_until_end_of_buffer;
	char *peek_ptr;
	long mx_status_code;

	if ( buffer == (MX_CIRCULAR_BUFFER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_CIRCULAR_BUFFER pointer passed was NULL." );
	}
	if ( data_destination == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The data_destination pointer passed was NULL." );
	}

	/* Lock the mutex. */

	mx_status_code = mx_mutex_lock( buffer->mutex );

	if ( mx_status_code != MXE_SUCCESS ) {
		return mx_error( mx_status_code, fname,
		"The attempt to lock MX_CIRCULAR_BUFFER %p failed.",
			buffer );
	}

	/* How many bytes are available to peek from the buffer?
	 *
	 * If bytes_read > bytes_written due to bytes_written wrapping
	 * around at ULONG_MAX, then the underflow due to the subtraction
	 * will still produce the correct value of num_bytes_in_use
	 * as long as buffer overrun has not happened.  This relies on
	 * the fact that mx_circular_buffer_write() is designed to
	 * prevent buffer overrun from happening.
	 */

	num_bytes_in_use = buffer->bytes_written - buffer->bytes_read;

	if ( num_bytes_in_use >= max_bytes_to_peek ) {
		num_bytes_to_peek = max_bytes_to_peek;
	} else {
		num_bytes_to_peek = num_bytes_in_use;
	}

	bytes_read_modulo = buffer->bytes_read % ( buffer->buffer_size );

	/* Does the region we need to read from wrap around to the start
	 * of the buffer?  If so, then we must break up the data copy
	 * operation into two separate copies.
	 */

	future_value_of_bytes_read = buffer->bytes_read + num_bytes_to_peek;

	future_value_of_bytes_read_modulo
		= future_value_of_bytes_read % ( buffer->buffer_size );

	if ( future_value_of_bytes_read_modulo >= bytes_read_modulo ) {
		/* This case we can do as one copy. */

		peek_ptr = buffer->data_array + bytes_read_modulo;

		memcpy( data_destination, peek_ptr, num_bytes_to_peek );
	} else {
		/* This case must be split into two copies. */

		peek_ptr = buffer->data_array + bytes_read_modulo;

		bytes_until_end_of_buffer
			= buffer->buffer_size - bytes_read_modulo;

		/* This is the part at the end of the buffer. */

		memcpy( data_destination, peek_ptr, bytes_until_end_of_buffer );

		/* This is the remaining part that has wrapped around
		 * back to the beginning of the buffer.
		 */

		memcpy( data_destination + bytes_until_end_of_buffer,
				buffer->data_array,
				future_value_of_bytes_read_modulo );
	}

	if ( num_bytes_peeked != (unsigned long *) NULL ) {
		*num_bytes_peeked = num_bytes_to_peek;
	}

	mx_status_code = mx_mutex_unlock( buffer->mutex );

	if ( mx_status_code != MXE_SUCCESS ) {
		return mx_error( mx_status_code, fname,
		"The attempt to unlock MX_CIRCULAR_BUFFER %p failed.",
			buffer );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_circular_buffer_write( MX_CIRCULAR_BUFFER *buffer,
			char *data_source,
			unsigned long max_bytes_to_write,
			unsigned long *num_bytes_written )
{
	static const char fname[] = "mx_circular_buffer_write()";

	unsigned long num_bytes_in_use, num_unused_bytes;
	unsigned long num_bytes_to_write;
	unsigned long bytes_written_modulo;
	unsigned long future_value_of_bytes_written;
	unsigned long future_value_of_bytes_written_modulo;
	unsigned long bytes_until_end_of_buffer;
	char *write_ptr;
	long mx_status_code;

	if ( buffer == (MX_CIRCULAR_BUFFER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_CIRCULAR_BUFFER pointer passed was NULL." );
	}
	if ( data_source == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The data_source pointer passed was NULL." );
	}

	/* Lock the mutex. */

	mx_status_code = mx_mutex_lock( buffer->mutex );

	if ( mx_status_code != MXE_SUCCESS ) {
		return mx_error( mx_status_code, fname,
		"The attempt to lock MX_CIRCULAR_BUFFER %p failed.",
			buffer );
	}

	num_bytes_in_use = buffer->bytes_written - buffer->bytes_read;

	if ( num_bytes_in_use > buffer->buffer_size ) {
		return mx_error( MXE_LIMIT_WAS_EXCEEDED, fname,
		"Buffer overrun for MX_CIRCULAR_BUFFER %p.  "
		"The difference (%lu) between bytes_written (%lu) "
		"and bytes_read (%lu) is larger than the "
		"total buffer size (%lu).  "
		"This should not be able to happen.",
			buffer,
			num_bytes_in_use,
			buffer->bytes_written,
			buffer->bytes_read,
			buffer->buffer_size );
	}

	num_unused_bytes = buffer->buffer_size - num_bytes_in_use;

	if ( num_unused_bytes >= max_bytes_to_write ) {
		num_bytes_to_write = max_bytes_to_write;
	} else {
		num_bytes_to_write = num_unused_bytes;
	}

	bytes_written_modulo = buffer->bytes_written % ( buffer->buffer_size );

	/* Does the region we need to write to wrap around to the start
	 * of the buffer?  If so, then we must break up the data copy
	 * operation into two separate copies.
	 */

	future_value_of_bytes_written
		= buffer->bytes_written + num_bytes_to_write;

	future_value_of_bytes_written_modulo
		= future_value_of_bytes_written % ( buffer->buffer_size );

	if ( future_value_of_bytes_written_modulo >= bytes_written_modulo ) {
		/* This case we can do as one copy. */

		write_ptr = buffer->data_array + bytes_written_modulo;

		memcpy( write_ptr, data_source, num_bytes_to_write );
	} else {
		/* This case must be split into two copies. */

		write_ptr = buffer->data_array + bytes_written_modulo;

		bytes_until_end_of_buffer
			= buffer->buffer_size - bytes_written_modulo;

		/* This is the part at the end of the buffer. */

		memcpy( write_ptr, data_source, bytes_until_end_of_buffer );

		/* This is the remaining part that has wrapped around
		 * back to the beginning of the buffer.
		 */

		memcpy( buffer->data_array,
				data_source + bytes_until_end_of_buffer,
				future_value_of_bytes_written_modulo );
	}

	buffer->bytes_written = future_value_of_bytes_written;

	if ( num_bytes_written != (unsigned long *) NULL ) {
		*num_bytes_written = num_bytes_to_write;
	}

	mx_status_code = mx_mutex_unlock( buffer->mutex );

	if ( mx_status_code != MXE_SUCCESS ) {
		return mx_error( mx_status_code, fname,
		"The attempt to unlock MX_CIRCULAR_BUFFER %p failed.",
			buffer );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_circular_buffer_num_bytes_available( MX_CIRCULAR_BUFFER *buffer,
					unsigned long *num_bytes_available )
{
	static const char fname[] = "mx_circular_buffer_num_bytes_available()";

	unsigned long num_bytes_in_use;
	long mx_status_code;

	if ( buffer == (MX_CIRCULAR_BUFFER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_CIRCULAR_BUFFER pointer passed was NULL." );
	}
	if ( num_bytes_available == (unsigned long *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The num_bytes_available pointer passed was NULL." );
	}

	/* Lock the mutex */

	mx_status_code = mx_mutex_lock( buffer->mutex );

	if ( mx_status_code != MXE_SUCCESS ) {
		return mx_error( mx_status_code, fname,
		"The attempt to lock MX_CIRCULAR_BUFFER %p failed.",
			buffer );
	}

	num_bytes_in_use = buffer->bytes_written - buffer->bytes_read;

	/* Unlock the mutex */

	mx_status_code = mx_mutex_unlock( buffer->mutex );

	if ( mx_status_code != MXE_SUCCESS ) {
		return mx_error( mx_status_code, fname,
		"The attempt to unlock MX_CIRCULAR_BUFFER %p failed.",
			buffer );
	}

	*num_bytes_available = num_bytes_in_use;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_circular_buffer_increment_read_bytes( MX_CIRCULAR_BUFFER *buffer,
					unsigned long num_bytes_to_increment )
{
	static const char fname[] = "mx_circular_buffer_increment_read_bytes()";

	long mx_status_code;

	if ( buffer == (MX_CIRCULAR_BUFFER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_CIRCULAR_BUFFER pointer passed was NULL." );
	}

	/* Lock the mutex */

	mx_status_code = mx_mutex_lock( buffer->mutex );

	if ( mx_status_code != MXE_SUCCESS ) {
		return mx_error( mx_status_code, fname,
		"The attempt to lock MX_CIRCULAR_BUFFER %p failed.",
			buffer );
	}

	buffer->bytes_read += num_bytes_to_increment;

	/* Unlock the mutex */

	mx_status_code = mx_mutex_unlock( buffer->mutex );

	if ( mx_status_code != MXE_SUCCESS ) {
		return mx_error( mx_status_code, fname,
		"The attempt to unlock MX_CIRCULAR_BUFFER %p failed.",
			buffer );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_circular_buffer_discard_available_bytes( MX_CIRCULAR_BUFFER *buffer )
{
	static const char fname[] =
		"mx_circular_buffer_discard_available_bytes()";

	long mx_status_code;

	if ( buffer == (MX_CIRCULAR_BUFFER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_CIRCULAR_BUFFER pointer passed was NULL." );
	}

	/* Lock the mutex */

	mx_status_code = mx_mutex_lock( buffer->mutex );

	if ( mx_status_code != MXE_SUCCESS ) {
		return mx_error( mx_status_code, fname,
		"The attempt to lock MX_CIRCULAR_BUFFER %p failed.",
			buffer );
	}

	/* All we need to do here is to declare all of the unread bytes
	 * as read.
	 */

	buffer->bytes_written = buffer->bytes_read;

	/* Unlock the mutex */

	mx_status_code = mx_mutex_unlock( buffer->mutex );

	if ( mx_status_code != MXE_SUCCESS ) {
		return mx_error( mx_status_code, fname,
		"The attempt to unlock MX_CIRCULAR_BUFFER %p failed.",
			buffer );
	}

	return MX_SUCCESSFUL_RESULT;
}

