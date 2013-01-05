/*
 * Name:    d_k8055_aoutput.h
 *
 * Purpose: Header file for Velleman K8055 analog output channels.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2013 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_K8055_AOUTPUT_H__
#define __D_K8055_AOUTPUT_H__

#include "mx_analog_output.h"

typedef struct {
	MX_RECORD *record;

	MX_RECORD *k8055_record;
	unsigned long channel;

} MX_K8055_AOUTPUT;

MX_API mx_status_type mxd_k8055_aoutput_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxd_k8055_aoutput_write(MX_ANALOG_OUTPUT *aoutput);

extern MX_RECORD_FUNCTION_LIST mxd_k8055_aoutput_record_function_list;
extern MX_ANALOG_OUTPUT_FUNCTION_LIST
		mxd_k8055_aoutput_analog_output_function_list;

extern long mxd_k8055_aoutput_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_k8055_aoutput_rfield_def_ptr;

#define MXD_K8055_AOUTPUT_STANDARD_FIELDS \
  {-1, -1, "k8055_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_K8055_AOUTPUT, k8055_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "channel", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_K8055_AOUTPUT, channel), \
	{0}, NULL, MXFF_IN_DESCRIPTION }

#endif /* __D_K8055_AOUTPUT_H__ */
