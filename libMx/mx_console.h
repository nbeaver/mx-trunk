/*
 * Name:    mx_console.h
 *
 * Purpose: Console handling functions.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2009 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_CONSOLE_H__
#define __MX_CONSOLE_H__

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

MX_API mx_status_type mx_get_console_size( unsigned long *num_rows,
					unsigned long *num_columns );

MX_API mx_status_type mx_paged_console_output( FILE *console,
						char *output_text );

#ifdef __cplusplus
}
#endif

#endif /* __MX_CONSOLE_H__ */

