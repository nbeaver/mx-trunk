/*
 * Name:    mx_export.h
 *
 * Purpose: Header file for exports to other control systems.
 *
 * Author:  William Lavender
 *
 *----------------------------------------------------------------------
 *
 * Copyright 2009 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_EXPORT_H__
#define __MX_EXPORT_H__

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

MX_API mx_status_type mx_register_export_callback(
			char *export_typename,
			mx_status_type (*callback_fn)( MX_RECORD *, char * ) );

MX_API mx_status_type mx_invoke_export_callback( MX_RECORD *record_list,
					char *record_description_buffer );

#ifdef __cplusplus
}
#endif

#endif /* __MX_EXPORT_H__ */

