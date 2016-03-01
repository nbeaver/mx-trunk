/*
 * Name:    mx_io.h
 *
 * Purpose: MX-specific file/pipe/etc I/O related functions.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2010, 2012-2013, 2015-2016 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef _MX_IO_H_
#define _MX_IO_H_

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

#include "mx_socket.h"
#include "mx_stdint.h"

/* mx_fd_num_input_bytes_available() tells us how many bytes can be read
 * from a Unix-style file descriptor without blocking.
 */

MX_API mx_status_type
mx_fd_num_input_bytes_available( int file_descriptor,
				size_t *num_bytes_available );

MX_API int mx_get_max_file_descriptors( void );

MX_API int mx_get_number_of_open_file_descriptors( void );

MX_API mx_bool_type mx_fd_is_valid( int fd );

/*----*/

MX_API int64_t mx_get_file_size( char *filename );

MX_API int64_t mx_get_file_offset( FILE *mx_file );

MX_API int64_t mx_set_file_offset( FILE *mx_file,
				int64_t file_offset,
				int origin );

/*----*/

/*
 * mx_get_disk_space() only reports on disk space available to 
 * the calling user.  If user quotas are in effect, these values
 * may be smaller than the values for the whole disk.
 */

MX_API mx_status_type mx_get_disk_space( char *filename,
				uint64_t *user_total_bytes_in_partition,
				uint64_t *user_free_bytes_in_partition );

/*----*/

MX_API char *mx_get_fd_name( unsigned long process_id, int fd,
				char *buffer, size_t buffer_size );

MX_API void mx_show_fd_names( unsigned long process_id );

/*----*/

MX_API void mx_win32_show_socket_names( void );

/*----*/

/* Generally, MX uses the Posix-style access type flags R_OK, W_OK, and X_OK.
 *
 * However, the access_type argument to mx_create_file_monitor() supports
 * additional flags:
 *
 * MXF_FILE_MONITOR_QUIET - Suppress file access error messages.
 */

#define MXF_FILE_MONITOR_QUIET	0x10000000

typedef struct {
	unsigned long access_type;
	char filename[MXU_FILENAME_LENGTH+1];

	void *private_ptr;
} MX_FILE_MONITOR;

MX_API mx_status_type mx_create_file_monitor( MX_FILE_MONITOR **ptr_address,
						unsigned long access_type,
						const char *filename );

MX_API mx_status_type mx_delete_file_monitor( MX_FILE_MONITOR *file_monitor );

MX_API mx_bool_type mx_file_has_changed( MX_FILE_MONITOR *file_monitor );

/*----*/

#ifdef __cplusplus
}
#endif

#endif /* _MX_IO_H_ */

