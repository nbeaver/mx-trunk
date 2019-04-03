/*
 * Name:     n_http.h
 *
 * Purpose:  Header file for client interface to a HTTP-based microcontroller.
 *
 * Author:   William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2019 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __N_HTTP_H__
#define __N_HTTP_H__

#include "mx_socket.h"

/* ===== HTTP server record structure ===== */

typedef struct {
	MX_RECORD *record;

	MX_RECORD *rs232_record;
} MX_HTTP_SERVER;

/* Define all of the client interface functions. */

MX_API mx_status_type mxn_http_server_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxn_http_server_open( MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxn_http_server_record_function_list;

extern long mxn_http_server_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxn_http_server_rfield_def_ptr;

#define MXN_HTTP_SERVER_STANDARD_FIELDS \
  {-1, -1, "rs232_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_HTTP_SERVER, rs232_record), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __N_HTTP_H__ */

