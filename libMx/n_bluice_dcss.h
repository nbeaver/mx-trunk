/*
 * Name:     n_bluice_dcss.h
 *
 * Purpose:  Header file for client interface to a Blu-Ice DCSS server.
 *
 * Author:   William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __N_BLUICE_DCSS_H__
#define __N_BLUICE_DCSS_H__

#include "mx_socket.h"

#define MXF_BLUICE_DCSS_AUTO_TAKE_MASTER	0x1

/* ===== Blu-Ice DCSS server record data structures ===== */

#define MXU_SESSION_ID_LENGTH	100

typedef struct {
	MX_RECORD *record;

	char hostname[MXU_HOSTNAME_LENGTH+1];
	int port_number;
	char session_id[MXU_SESSION_ID_LENGTH+1];
	unsigned long bluice_dcss_flags;

	MX_THREAD *dcss_monitor_thread;
	unsigned long client_number;
	int is_master;
} MX_BLUICE_DCSS_SERVER;

/* Define all of the client interface functions. */

MX_API mx_status_type mxn_bluice_dcss_server_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxn_bluice_dcss_server_delete_record( MX_RECORD *record );
MX_API mx_status_type mxn_bluice_dcss_server_open( MX_RECORD *record );
MX_API mx_status_type mxn_bluice_dcss_server_close( MX_RECORD *record );
MX_API mx_status_type mxn_bluice_dcss_server_resynchronize( MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxn_bluice_dcss_server_record_function_list;

extern long mxn_bluice_dcss_server_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxn_bluice_dcss_server_rfield_def_ptr;

#define MXN_BLUICE_DCSS_SERVER_STANDARD_FIELDS \
  {-1, -1, "hostname", MXFT_STRING, NULL, 1, {MXU_HOSTNAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_BLUICE_DCSS_SERVER, hostname), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "port_number", MXFT_INT, NULL, 0, {0}, \
  	MXF_REC_TYPE_STRUCT, offsetof(MX_BLUICE_DCSS_SERVER, port_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "session_id", MXFT_STRING, NULL, 1, {MXU_SESSION_ID_LENGTH}, \
  	MXF_REC_TYPE_STRUCT, offsetof(MX_BLUICE_DCSS_SERVER, session_id), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "bluice_dcss_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_BLUICE_DCSS_SERVER, bluice_dcss_flags), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __N_BLUICE_DCSS_H__ */

