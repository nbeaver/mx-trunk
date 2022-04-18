/*
 * Name:    mx_vm_alloc.h
 *
 * Purpose: Header for MX functions that allocate virtual memory directly
 *          from the underlying memory manager.  Read, write, and execute
 *          permissions can be set for this memory.  Specifying a requested
 *          address of NULL lets the operating system select the actual
 *          address.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2013, 2021-2022 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_VM_ALLOC_H__
#define __MX_VM_ALLOC_H__

#include "mx_stdint.h"

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

/* The flag bit values for 'protection_flags' are the R_OK, W_OK, and X_OK
 * values defined either in <unistd.h> or "mx_unistd.h".
 */

MX_API void *mx_vm_alloc( void *requested_address,
			size_t requested_size_in_bytes,
			unsigned long protection_flags );

MX_API void mx_vm_free( void *address );

MX_API mx_status_type mx_vm_get_protection( void *address,
					size_t range_size_in_bytes,
					mx_bool_type *valid_address_range,
					unsigned long *protection_flags );

MX_API mx_status_type mx_vm_get_region( void *address,
					size_t range_size_in_bytes,
					mx_bool_type *valid_address_range,
					unsigned long *protection_flags,
					void **region_base_address,
					size_t *region_size_in_bytes );

MX_API mx_status_type mx_vm_set_protection( void *address,
					size_t range_size_in_bytes,
					unsigned long protection_flags );

MX_API mx_status_type mx_vm_show_os_info( FILE *file,
					char *id_string,
					void *address,
					size_t range_size_in_bytes );

#ifdef __cplusplus
}
#endif

#endif /* __MX_VM_ALLOC_H__ */

