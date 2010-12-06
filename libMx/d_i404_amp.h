/*
 * Name:    d_i404_amp.h
 *
 * Purpose: Header for the Pyramid Technical Consultants I404
 *          digital electrometer.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_I404_AMP_H__
#define __D_I404_AMP_H__

typedef struct {
	MX_RECORD *record;

	MX_RECORD *i404_record;
} MX_I404_AMP;

#define MXD_I404_AMP_STANDARD_FIELDS \
  {-1, -1, "i404_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_I404_AMP, i404_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

MX_API mx_status_type mxd_i404_amp_create_record_structures(MX_RECORD *record);

MX_API mx_status_type mxd_i404_amp_get_gain( MX_AMPLIFIER *amplifier );
MX_API mx_status_type mxd_i404_amp_set_gain( MX_AMPLIFIER *amplifier );

extern MX_RECORD_FUNCTION_LIST mxd_i404_amp_record_function_list;

extern MX_AMPLIFIER_FUNCTION_LIST mxd_i404_amp_amplifier_function_list;

extern long mxd_i404_amp_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_i404_amp_rfield_def_ptr;

#endif /* __D_I404_AMP_H__ */

