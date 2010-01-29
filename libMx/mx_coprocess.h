/*
 * Name:    mx_coprocess.h
 *
 * Purpose: Create a coprocess connected to the current process by a pair
 *          of pipes.  One pipe is used to send to the coprocess and the
 *          other pipe is used to receive from the coprocess.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2003, 2007, 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_COPROCESS_H__
#define __MX_COPROCESS_H__

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	FILE *from_coprocess;
	FILE *to_coprocess;
	unsigned long coprocess_pid;

	void *private;
} MX_COPROCESS;

/* Bitmap bits for the 'flags' argument. */

#define MXF_CP_CREATE_PROCESS_GROUP	0x1

/*---*/

MX_API mx_status_type mx_coprocess_open( MX_COPROCESS **coprocess,
						char *command_line,
						unsigned long flags );

MX_API mx_status_type mx_coprocess_close( MX_COPROCESS *coprocess,
						double timeout );

MX_API mx_status_type mx_coprocess_num_bytes_available(
						MX_COPROCESS *coprocess,
						size_t *num_bytes_available );

#ifdef __cplusplus
}
#endif

#endif /* __MX_COPROCESS_H__ */

