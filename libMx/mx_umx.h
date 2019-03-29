/*
 * Name: mx_umx.c
 *
 * Purpose: Header file for communication with a UMX-based microcontroller.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2019 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_UMX_H__
#define __MX_UMX_H__

#ifdef __cplusplus
extern "C" {
#endif

MX_API mx_status_type
mx_umx_command( MX_RECORD *umx_record,
		char *command,
		char *response,
		size_t max_response_length,
		mx_bool_type debug_flag );

#ifdef __cplusplus
}
#endif

#endif /* __MX_UMX_H__ */
