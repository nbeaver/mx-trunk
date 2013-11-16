/*
 * Name:    mx_circular_buffer.h
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

#ifndef __MX_CIRCULAR_BUFFER_H__
#define __MX_CIRCULAR_BUFFER_H__

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

/* Note: As long as the infinite precision equivalents of 'bytes_read'
 *       and 'bytes_written' obey the relationship
 *
 *           ( bytes_written - bytes_read ) < ULONG_MAX
 *
 *       then read and write index computations will do the right thing
 *       even in the presence of wraparound at ULONG_MAX.
 */

typedef struct {
	MX_MUTEX *mutex;
	unsigned long buffer_size;
	unsigned long bytes_read;
	unsigned long bytes_written;
	char *data_array;
} MX_CIRCULAR_BUFFER;

MX_API mx_status_type mx_circular_buffer_create( MX_CIRCULAR_BUFFER **buffer,
						unsigned long buffer_size );

MX_API mx_status_type mx_circular_buffer_destroy( MX_CIRCULAR_BUFFER *buffer );

MX_API mx_status_type mx_circular_buffer_read( MX_CIRCULAR_BUFFER *buffer,
					char *data_destination,
					unsigned long max_bytes_to_read,
					unsigned long *num_bytes_read );

MX_API mx_status_type mx_circular_buffer_peek( MX_CIRCULAR_BUFFER *buffer,
					char *data_destination,
					unsigned long max_bytes_to_peek,
					unsigned long *num_bytes_peeked );

MX_API mx_status_type mx_circular_buffer_write( MX_CIRCULAR_BUFFER *buffer,
					char *data_source,
					unsigned long max_bytes_to_write,
					unsigned long *num_bytes_written );
						
MX_API mx_status_type mx_circular_buffer_num_bytes_available(
					MX_CIRCULAR_BUFFER *buffer,
					unsigned long *num_bytes_available );

MX_API mx_status_type mx_circular_buffer_increment_bytes_read(
					MX_CIRCULAR_BUFFER *buffer,
					unsigned long num_bytes_to_increment );

MX_API mx_status_type mx_circular_buffer_discard_available_bytes(
					MX_CIRCULAR_BUFFER *buffer );
						

#ifdef __cplusplus
}
#endif

#endif /* __MX_CIRCULAR_BUFFER_H__ */
