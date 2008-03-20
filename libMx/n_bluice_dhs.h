/*
 * Name:     n_bluice_dhs.h
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

#ifndef __N_BLUICE_DHS_H__
#define __N_BLUICE_DHS_H__

#define MXU_BLUICE_DHS_NAME_LENGTH	40

/* Flag values for the 'bluice_dhs_flags' field. */

#define MXF_BLUICE_DHS_DUMMY	0x1

/* ===== Blu-Ice DHS server record data structures ===== */

typedef struct {
	MX_RECORD *record;

	MX_RECORD *dhs_manager_record;
	char dhs_name[MXU_BLUICE_DHS_NAME_LENGTH+1];
	unsigned long bluice_dhs_flags;

	MX_THREAD *dhs_monitor_thread;
} MX_BLUICE_DHS_SERVER;

/* Define all of the client interface functions. */

MX_API mx_status_type mxn_bluice_dhs_server_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxn_bluice_dhs_server_delete_record( MX_RECORD *record );
MX_API mx_status_type mxn_bluice_dhs_server_open( MX_RECORD *record );
MX_API mx_status_type mxn_bluice_dhs_server_close( MX_RECORD *record );
MX_API mx_status_type mxn_bluice_dhs_server_resynchronize( MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxn_bluice_dhs_server_record_function_list;

extern long mxn_bluice_dhs_server_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxn_bluice_dhs_server_rfield_def_ptr;

#define MXN_BLUICE_DHS_SERVER_STANDARD_FIELDS \
  {-1, -1, "dhs_manager_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_BLUICE_DHS_SERVER, dhs_manager_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "dhs_name", MXFT_STRING, NULL, 1, {MXU_BLUICE_DHS_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_BLUICE_DHS_SERVER, dhs_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "bluice_dhs_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_BLUICE_DHS_SERVER, bluice_dhs_flags), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __N_BLUICE_DHS_H__ */

