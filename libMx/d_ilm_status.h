/*
 * Name:    d_ilm_status.h
 *
 * Purpose: Header file for reading status values from Oxford Instruments
 *          ILM (Intelligent Level Meter) controllers.
 *
 * Author:  William Lavender / Henry Bellamy
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2007-2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_ILM_STATUS_H__
#define __D_ILM_STATUS_H__

typedef struct {
	MX_RECORD *record;

	MX_RECORD *ilm_record;
	long channel;
} MX_ILM_STATUS;

#define MXD_ILM_STATUS_STANDARD_FIELDS \
  {-1, -1, "ilm_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ILM_STATUS, ilm_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "channel", MXFT_LONG, NULL, 0, {0}, \
  	MXF_REC_TYPE_STRUCT, offsetof(MX_ILM_STATUS, channel), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

MX_API mx_status_type mxd_ilm_status_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_ilm_status_open( MX_RECORD *record );

MX_API mx_status_type mxd_ilm_status_read( MX_DIGITAL_INPUT *status );

extern MX_RECORD_FUNCTION_LIST mxd_ilm_status_record_function_list;

extern MX_DIGITAL_INPUT_FUNCTION_LIST
				mxd_ilm_status_digital_input_function_list;

extern long mxd_ilm_status_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_ilm_status_rfield_def_ptr;

#endif /* __D_ILM_STATUS_H__ */

