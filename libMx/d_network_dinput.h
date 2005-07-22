/*
 * Name:    d_network_dinput.h
 *
 * Purpose: Header file for MX network digital input devices.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2004 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_NETWORK_DINPUT_H__
#define __D_NETWORK_DINPUT_H__

#include "mx_digital_input.h"

typedef struct {
	MX_RECORD *server_record;
	char remote_record_name[ MXU_RECORD_NAME_LENGTH+1 ];

	MX_NETWORK_FIELD value_nf;
} MX_NETWORK_DINPUT;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_network_dinput_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_network_dinput_finish_record_initialization(
							MX_RECORD *record );

MX_API mx_status_type mxd_network_dinput_read( MX_DIGITAL_INPUT *dinput );

extern MX_RECORD_FUNCTION_LIST mxd_network_dinput_record_function_list;
extern MX_DIGITAL_INPUT_FUNCTION_LIST
				mxd_network_dinput_digital_input_function_list;

extern long mxd_network_dinput_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_network_dinput_rfield_def_ptr;

#define MXD_NETWORK_DINPUT_STANDARD_FIELDS \
  {-1, -1, "server_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NETWORK_DINPUT, server_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "remote_record_name", MXFT_STRING, \
	NULL, 1, {MXU_RECORD_NAME_LENGTH}, \
     MXF_REC_TYPE_STRUCT, offsetof(MX_NETWORK_DINPUT, remote_record_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

#endif /* __D_NETWORK_DINPUT_H__ */

