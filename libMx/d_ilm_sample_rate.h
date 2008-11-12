/*
 * Name:    d_ilm_sample_rate.h
 *
 * Purpose: Header file for changing the sample rate of an Oxford Instruments
 *          ILM (Intelligent Level Meter) controllers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_ILM_SAMPLE_RATE_H__
#define __D_ILM_SAMPLE_RATE_H__

typedef struct {
	MX_RECORD *record;

	MX_RECORD *ilm_record;
	long channel;
} MX_ILM_SAMPLE_RATE;

#define MXD_ILM_SAMPLE_RATE_STANDARD_FIELDS \
  {-1, -1, "ilm_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ILM_SAMPLE_RATE, ilm_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "channel", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ILM_SAMPLE_RATE, channel), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

MX_API mx_status_type mxd_ilm_sample_rate_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxd_ilm_sample_rate_open( MX_RECORD *record );

MX_API mx_status_type mxd_ilm_sample_rate_read( MX_DIGITAL_OUTPUT *doutput );
MX_API mx_status_type mxd_ilm_sample_rate_write( MX_DIGITAL_OUTPUT *doutput );

extern MX_RECORD_FUNCTION_LIST mxd_ilm_sample_rate_record_function_list;

extern MX_DIGITAL_OUTPUT_FUNCTION_LIST
		mxd_ilm_sample_rate_digital_output_function_list;

extern long mxd_ilm_sample_rate_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_ilm_sample_rate_rfield_def_ptr;

#endif /* __D_ILM_SAMPLE_RATE_H__ */

