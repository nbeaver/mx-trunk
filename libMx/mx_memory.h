/*
 * Name:    mx_memory.h
 *
 * Purpose: This header defines structures and functions used for
 *          evaluating the memory usage of a process or the system as
 *          a whole.  All values are expressed in kilobytes.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------
 *
 * Copyright 2005, 2007, 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_MEMORY_H__
#define __MX_MEMORY_H__

#include "mx_constants.h"

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

#define MXF_PROCESS_ID_SELF	MX_ULONG_MAX

typedef unsigned long mx_memsize_type;

typedef struct {
	unsigned long process_id;
	mx_memsize_type total_bytes;
	mx_memsize_type locked_in_memory_bytes;
	mx_memsize_type resident_in_memory_bytes;
	mx_memsize_type data_bytes;
	mx_memsize_type stack_bytes;
	mx_memsize_type heap_bytes;
	mx_memsize_type code_bytes;
	mx_memsize_type library_bytes;
	mx_memsize_type allocated_bytes;
	mx_memsize_type memory_mapped_bytes;
} MX_PROCESS_MEMINFO;

typedef struct {
	mx_memsize_type total_bytes;
	mx_memsize_type used_bytes;
	mx_memsize_type free_bytes;

	mx_memsize_type swap_bytes;
	mx_memsize_type swap_used_bytes;
	mx_memsize_type swap_free_bytes;
} MX_SYSTEM_MEMINFO;

MX_API mx_status_type mx_get_process_meminfo( unsigned long process_id,
						MX_PROCESS_MEMINFO *meminfo );

MX_API mx_status_type mx_get_system_meminfo( MX_SYSTEM_MEMINFO *meminfo );

#define MX_MEMORY_DISPLAY_COUNTER( x, y ) \
	do {                                                                  \
	    MX_PROCESS_MEMINFO temp_mi;                                       \
	    (void) mx_get_process_meminfo(MXF_PROCESS_ID_SELF, &temp_mi);     \
	    fprintf(stderr,"%s: Memory usage = %lu bytes\n", y, temp_mi.x);   \
	} while (0)

MX_API void mx_display_process_meminfo( MX_PROCESS_MEMINFO *meminfo );

MX_API void mx_display_system_meminfo( MX_SYSTEM_MEMINFO *meminfo );

/*---*/

MX_API void mx_show_process_total_bytes( void );
MX_API void mx_show_process_stack_bytes( void );
MX_API void mx_show_process_heap_bytes( void );
MX_API void mx_show_process_allocated_bytes( void );
						
#ifdef __cplusplus
}
#endif

#endif /* __MX_MEMORY_H__ */

