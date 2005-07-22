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
 * Copyright 1999-2001, 2003 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_COPROCESS_H__
#define __MX_COPROCESS_H__

typedef struct {
	FILE *from_coprocess;
	FILE *to_coprocess;
	pid_t coprocess_pid;
} MX_COPROCESS;

MX_API mx_status_type mx_create_coprocess( MX_COPROCESS *coprocess,
						char *command_line );

MX_API mx_status_type mx_kill_coprocess( MX_COPROCESS *coprocess );

#endif /* __MX_COPROCESS_H__ */

