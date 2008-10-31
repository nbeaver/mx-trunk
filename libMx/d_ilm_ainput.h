/*
 * Name:    d_ilm_ainput.h
 *
 * Purpose: Header file for values returned by the READ command for
 *          Oxford Instruments ILM (Intelligent Level Meter) controllers.
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

#ifndef __D_ILM_AINPUT_H__
#define __D_ILM_AINPUT_H__

typedef struct {
	MX_RECORD *record;

	MX_RECORD *ilm_record;
	long channel;
} MX_ILM_AINPUT;

#define MXD_ILM_AINPUT_STANDARD_FIELDS \
  {-1, -1, "ilm_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ILM_AINPUT, ilm_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "channel", MXFT_LONG, NULL, 0, {0}, \
  	MXF_REC_TYPE_STRUCT, offsetof(MX_ILM_AINPUT, channel), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

MX_API mx_status_type mxd_ilm_ainput_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_ilm_ainput_open( MX_RECORD *record );

MX_API mx_status_type mxd_ilm_ainput_read( MX_ANALOG_INPUT *ainput );

extern MX_RECORD_FUNCTION_LIST mxd_ilm_ainput_record_function_list;

extern MX_ANALOG_INPUT_FUNCTION_LIST mxd_ilm_ainput_analog_input_function_list;

extern long mxd_ilm_ainput_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_ilm_ainput_rfield_def_ptr;

#endif /* __D_ILM_AINPUT_H__ */

