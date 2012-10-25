/*
 * Name:    i_edt.h
 *
 * Purpose: Header file for EDT cameras.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_EDT_H__
#define __I_EDT_H__


typedef struct {
	MX_RECORD *record;

	char device_name[MXU_FILENAME_LENGTH+1];
} MX_EDT;

#define MXI_EDT_STANDARD_FIELDS \
  {-1, -1, "device_name", MXFT_STRING, NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EDT, device_name), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION}, \

MX_API mx_status_type mxi_edt_create_record_structures( MX_RECORD *record );

MX_API mx_status_type mxi_edt_open( MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxi_edt_record_function_list;

extern long mxi_edt_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_edt_rfield_def_ptr;

#endif /* __I_EDT_H__ */

