/*
 * Name:    d_spec_scaler.h
 *
 * Purpose: Header file for Spec scalers.
 *
 *-----------------------------------------------------------------------
 *
 * Copyright 2004 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_SPEC_SCALER_H__
#define __D_SPEC_SCALER_H__

typedef struct {
	MX_RECORD *spec_server_record;
	char remote_scaler_name[MXU_RECORD_NAME_LENGTH+1];
} MX_SPEC_SCALER;

MX_API mx_status_type mxd_spec_scaler_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxd_spec_scaler_read( MX_SCALER *scaler );

extern MX_RECORD_FUNCTION_LIST mxd_spec_scaler_record_function_list;
extern MX_SCALER_FUNCTION_LIST mxd_spec_scaler_scaler_function_list;

extern long mxd_spec_scaler_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_spec_scaler_rfield_def_ptr;

#define MXD_SPEC_SCALER_STANDARD_FIELDS \
  {-1, -1, "spec_server_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SPEC_SCALER, spec_server_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "remote_scaler_name", MXFT_STRING, \
				NULL, 1, {MXU_RECORD_NAME_LENGTH}, \
     MXF_REC_TYPE_STRUCT, offsetof(MX_SPEC_SCALER, remote_scaler_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

#endif /* __D_SPEC_SCALER_H__ */
