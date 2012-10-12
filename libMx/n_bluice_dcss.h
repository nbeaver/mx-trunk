/*
 * Name:     n_bluice_dcss.h
 *
 * Purpose:  Header file for client interface to a Blu-Ice DCSS server.
 *
 * Author:   William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2005-2006, 2008, 2012 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __N_BLUICE_DCSS_H__
#define __N_BLUICE_DCSS_H__

#include "mx_socket.h"

/* Flag values for the 'bluice_dcss_flags' field. */

#define MXF_BLUICE_DCSS_AUTO_TAKE_MASTER	0x1
#define MXF_BLUICE_DCSS_REQUIRE_USERNAME	0x2
#define MXF_BLUICE_DCSS_REQUIRE_PASSWORD	0x4

/* ===== Blu-Ice DCSS server record data structures ===== */

#define MXU_APPNAME_LENGTH		40
#define MXU_AUTHENTICATION_DATA_LENGTH	100

typedef struct {
	MX_RECORD *record;

	char hostname[MXU_HOSTNAME_LENGTH+1];
	long port_number;
	char appname[MXU_APPNAME_LENGTH+1];
	char authentication_data[MXU_AUTHENTICATION_DATA_LENGTH+1];
	unsigned long bluice_dcss_flags;

	MX_THREAD *dcss_monitor_thread;
	unsigned long client_number;
	mx_bool_type is_authenticated;
	mx_bool_type is_master;
	char bluice_username[MXU_USERNAME_LENGTH+1];

	uint32_t operation_counter;
} MX_BLUICE_DCSS_SERVER;

/* Define all of the client interface functions. */

MX_API mx_status_type mxn_bluice_dcss_server_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxn_bluice_dcss_server_delete_record( MX_RECORD *record );
MX_API mx_status_type mxn_bluice_dcss_server_print_structure( FILE *file,
							MX_RECORD *record );
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
  {-1, -1, "port_number", MXFT_LONG, NULL, 0, {0}, \
  	MXF_REC_TYPE_STRUCT, offsetof(MX_BLUICE_DCSS_SERVER, port_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "appname", MXFT_STRING, NULL, 1, {MXU_APPNAME_LENGTH}, \
  	MXF_REC_TYPE_STRUCT, offsetof(MX_BLUICE_DCSS_SERVER, appname), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "authentication_data", MXFT_STRING, \
  		NULL, 1, {MXU_AUTHENTICATION_DATA_LENGTH}, \
  	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_BLUICE_DCSS_SERVER, authentication_data), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "bluice_dcss_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_BLUICE_DCSS_SERVER, bluice_dcss_flags), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __N_BLUICE_DCSS_H__ */

