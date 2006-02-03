/*
 * Name:     n_spec.h
 *
 * Purpose:  Header file for client interface to a Spec server.
 *
 * Author:   William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __N_SPEC_H__
#define __N_SPEC_H__

#include "mx_socket.h"

/* ===== Spec server record data structures ===== */

typedef struct {
	MX_RECORD *record;

	/* server_id takes the form of 'hostname:program_id' where
	 * program_id is either a TCP/IP port number or the name
	 * of a running spec process as described in the section
	 * 'Configuring the spec Client' on the web page
	 * http://www.certif.com/spec_help/server.html
	 */

	char server_id[ MXU_HOSTNAME_LENGTH + 21 ];

	MX_SOCKET *socket;

} MX_SPEC_SERVER;

/* Define all of the client interface functions. */

MX_API mx_status_type mxn_spec_server_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxn_spec_server_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxn_spec_server_delete_record( MX_RECORD *record );
MX_API mx_status_type mxn_spec_server_open( MX_RECORD *record );
MX_API mx_status_type mxn_spec_server_close( MX_RECORD *record );
MX_API mx_status_type mxn_spec_server_resynchronize( MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxn_spec_server_record_function_list;

extern mx_length_type mxn_spec_server_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxn_spec_server_rfield_def_ptr;

#define MXN_SPEC_SERVER_STANDARD_FIELDS \
  {-1, -1, "server_id", MXFT_STRING, NULL, 1, { MXU_HOSTNAME_LENGTH + 20 }, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SPEC_SERVER, server_id), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __N_SPEC_H__ */

