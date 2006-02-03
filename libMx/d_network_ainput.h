/*
 * Name:    d_network_ainput.h
 *
 * Purpose: Header file for MX network analog input devices.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_NETWORK_AINPUT_H__
#define __D_NETWORK_AINPUT_H__

#include "mx_analog_input.h"

typedef struct {
	MX_RECORD *server_record;
	char remote_record_name[ MXU_RECORD_NAME_LENGTH+1 ];

	MX_NETWORK_FIELD dark_current_nf;
	MX_NETWORK_FIELD value_nf;
} MX_NETWORK_AINPUT;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_network_ainput_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_network_ainput_finish_record_initialization(
							MX_RECORD *record );

MX_API mx_status_type mxd_network_ainput_read( MX_ANALOG_INPUT *ainput );
MX_API mx_status_type mxd_network_ainput_get_dark_current(
						MX_ANALOG_INPUT *ainput );
MX_API mx_status_type mxd_network_ainput_set_dark_current(
						MX_ANALOG_INPUT *ainput );

extern MX_RECORD_FUNCTION_LIST mxd_network_ainput_record_function_list;
extern MX_ANALOG_INPUT_FUNCTION_LIST
				mxd_network_ainput_analog_input_function_list;

extern mx_length_type mxd_network_ainput_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_network_ainput_rfield_def_ptr;

#define MXD_NETWORK_AINPUT_STANDARD_FIELDS \
  {-1, -1, "server_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NETWORK_AINPUT, server_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "remote_record_name", MXFT_STRING, \
	NULL, 1, {MXU_RECORD_NAME_LENGTH}, \
     MXF_REC_TYPE_STRUCT, offsetof(MX_NETWORK_AINPUT, remote_record_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

#endif /* __D_NETWORK_AINPUT_H__ */

