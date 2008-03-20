/*
 * Name:     n_bluice_dhs_manager.h
 *
 * Purpose:  Header file for client interface to a Blu-Ice DHS server.
 *
 * Author:   William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __N_BLUICE_DHS_MANAGER_H__
#define __N_BLUICE_DHS_MANAGER_H__

/* Flag values for the 'bluice_dhs_manager_flags' field. */

#define MXF_BLUICE_DHS_MANAGER_DUMMY	0x1

/* ===== Blu-Ice DHS manager record data structures ===== */

typedef struct {
	MX_RECORD *record;

	long port_number;
	unsigned long bluice_dhs_manager_flags;

	MX_THREAD *dhs_manager_thread;
	MX_SOCKET *socket;

	char *receive_buffer;
	long receive_buffer_length;
	long num_received_bytes;
} MX_BLUICE_DHS_MANAGER;

/* Define all of the client interface functions. */

MX_API mx_status_type mxn_bluice_dhs_manager_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxn_bluice_dhs_manager_delete_record( MX_RECORD *record );
MX_API mx_status_type mxn_bluice_dhs_manager_open( MX_RECORD *record );
MX_API mx_status_type mxn_bluice_dhs_manager_close( MX_RECORD *record );
MX_API mx_status_type mxn_bluice_dhs_manager_resynchronize( MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxn_bluice_dhs_manager_record_function_list;

extern long mxn_bluice_dhs_manager_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxn_bluice_dhs_manager_rfield_def_ptr;

#define MXN_BLUICE_DHS_MANAGER_STANDARD_FIELDS \
  {-1, -1, "port_number", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_BLUICE_DHS_MANAGER, port_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "bluice_dhs_manager_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_BLUICE_DHS_MANAGER, bluice_dhs_manager_flags), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __N_BLUICE_DHS_MANAGER_H__ */

