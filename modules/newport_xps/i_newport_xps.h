/*
 * Name:    i_newport_xps.h
 *
 * Purpose: Header file for Newport XPS motor controllers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2014 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_NEWPORT_XPS_H__
#define __I_NEWPORT_XPS_H__

#include "mx_thread.h"
#include "mx_mutex.h"
#include "mx_condition_variable.h"


#define MXU_NEWPORT_USERNAME_LENGTH	40
#define MXU_NEWPORT_PASSWORD_LENGTH	40

typedef struct {
	MX_RECORD *record;

	char hostname[MXU_HOSTNAME_LENGTH+1];
	unsigned long port_number;

	char username[MXU_NEWPORT_USERNAME_LENGTH+1];
	char password[MXU_NEWPORT_PASSWORD_LENGTH+1];

	int socket_id;

	/* Move commands _block_, so they have to have their own
	 * separate socket and thread in order to avoid having
	 * the entire driver block during a move.
	 */

	int move_thread_socket_id;

	MX_THREAD *move_thread;
	MX_MUTEX *move_thread_mutex;
	MX_CONDITION_VARIABLE *move_thread_cv;
	char move_thread_command[100];
	mx_bool_type move_in_progress;
} MX_NEWPORT_XPS;

#define MXI_NEWPORT_XPS_STANDARD_FIELDS \
  {-1, -1, "hostname", MXFT_STRING, NULL, 1, {MXU_HOSTNAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NEWPORT_XPS, hostname), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "port_number", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NEWPORT_XPS, port_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "username", MXFT_STRING, NULL, 1, {MXU_NEWPORT_USERNAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NEWPORT_XPS, username), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_NO_ACCESS) }, \
  \
  {-1, -1, "password", MXFT_STRING, NULL, 1, {MXU_NEWPORT_PASSWORD_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NEWPORT_XPS, password), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_NO_ACCESS) }

MX_API mx_status_type mxi_newport_xps_create_record_structures(
						MX_RECORD *record );

MX_API mx_status_type mxi_newport_xps_open( MX_RECORD *record );

MX_API mx_status_type mxi_newport_xps_resynchronize( MX_RECORD *record );

MX_API mx_status_type mxi_newport_xps_error( MX_NEWPORT_XPS *newport_xps,
						char *api_name,
						int error_code );

extern MX_RECORD_FUNCTION_LIST mxi_newport_xps_record_function_list;

extern long mxi_newport_xps_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_newport_xps_rfield_def_ptr;

#endif /* __I_NEWPORT_XPS_H__ */

