/*
 * Name:    mx_io.h
 *
 * Purpose: MX-specific file/pipe/etc I/O related functions.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef _MX_IO_H_
#define _MX_IO_H_

/* mx_fd_num_input_bytes_available() tells us how many bytes can be read
 * from a Unix-style file descriptor without blocking.
 */

MX_API mx_status_type
mx_fd_num_input_bytes_available( int file_descriptor,
				size_t *num_bytes_available );

MX_API int mx_get_max_file_descriptors( void );

MX_API int mx_get_number_of_open_file_descriptors( void );

MX_API char *mx_get_fd_name( unsigned long process_id, int fd,
				char *buffer, size_t buffer_size );

MX_API void mx_show_fd_names( unsigned long process_id );

#endif /* _MX_IO_H_ */

