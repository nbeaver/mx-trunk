/*
 * Name:    mx_pipe.h
 *
 * Purpose: Header file for MX pipe functions.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_PIPE_H__
#define __MX_PIPE_H__

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

#define MXF_PIPE_READ	0x1
#define MXF_PIPE_WRITE	0x2

#define MXF_PIPE_BOTH	(MXF_PIPE_READ | MXF_PIPE_WRITE)

typedef struct {
	void *private_ptr;
} MX_PIPE;

MX_API mx_status_type mx_pipe_open( MX_PIPE **mx_pipe );

MX_API mx_status_type mx_pipe_close( MX_PIPE *mx_pipe, int flags );

MX_API mx_status_type mx_pipe_read( MX_PIPE *mx_pipe,
					char *buffer,
					size_t max_bytes_to_read,
					size_t *bytes_read );

MX_API mx_status_type mx_pipe_write( MX_PIPE *mx_pipe,
					char *buffer,
					size_t bytes_to_write );

MX_API mx_status_type mx_pipe_num_bytes_available( MX_PIPE *mx_pipe,
						size_t *num_bytes_available );

MX_API mx_status_type mx_pipe_set_blocking_mode( MX_PIPE *mx_pipe,
					int flags,
					mx_bool_type blocking_mode_flag );

#ifdef __cplusplus
}
#endif

#endif /* __MX_PIPE_H__ */

