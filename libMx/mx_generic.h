/*
 * Name:    mx_generic.h
 *
 * Purpose: Header file for generic interfaces that look similar to
 *          RS-232 or GPIB interfaces.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2005, 2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_GENERIC_H__
#define __MX_GENERIC_H__

#include "mx_record.h"

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

/* Used by mx_generic_getchar() and mx_generic_putchar(). */

#define MXF_GENERIC_WAIT	0
#define MXF_GENERIC_NOWAIT	1

/* ======== Generic interface data structure ======= */

typedef struct {
	MX_RECORD *record;
} MX_GENERIC;

/*
 * The structure type MX_GENERIC_FUNCTION_LIST contains a list of
 * pointers to functions that vary from interface type to
 * interface type.
 */

typedef struct {
	mx_status_type ( *getchar ) ( MX_GENERIC *generic, char *c, int flags );
	mx_status_type ( *putchar ) ( MX_GENERIC *generic, char c, int flags );
	mx_status_type ( *read ) ( MX_GENERIC *generic,
					void *buffer, size_t count );
	mx_status_type ( *write ) ( MX_GENERIC *generic,
					void *buffer, size_t count );
	mx_status_type ( *num_input_bytes_available ) ( MX_GENERIC *generic,
				unsigned long *num_input_bytes_available );
	mx_status_type ( *discard_unread_input ) ( MX_GENERIC *generic,
							int debug_flag );
	mx_status_type ( *discard_unwritten_output ) ( MX_GENERIC *generic,
							int debug_flag );
} MX_GENERIC_FUNCTION_LIST;

/* ============== Interface function prototypes. ============== */

MX_API mx_status_type mx_generic_getchar( MX_GENERIC *generic,
						char *c, int flags );

MX_API mx_status_type mx_generic_putchar( MX_GENERIC *generic,
						char c, int flags );

MX_API mx_status_type mx_generic_read( MX_GENERIC *generic,
						void *buffer, size_t count );

MX_API mx_status_type mx_generic_write( MX_GENERIC *generic,
						void *buffer, size_t count );

MX_API mx_status_type mx_generic_num_input_bytes_available( MX_GENERIC *generic,
				unsigned long *num_input_bytes_available );

MX_API mx_status_type mx_generic_discard_unread_input( MX_GENERIC *generic,
							int debug_flag );

MX_API mx_status_type mx_generic_discard_unwritten_output( MX_GENERIC *generic,
							int debug_flag );

extern MX_RECORD_FUNCTION_LIST  mx_generic_record_function_list;

#ifdef __cplusplus
}
#endif

#endif /* __MX_GENERIC_H__ */

