/*
 * Name:    d_k8055_dinput.h
 *
 * Purpose: Header file for Velleman K8055 digital input channels.
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

#ifndef __D_K8055_DINPUT_H__
#define __D_K8055_DINPUT_H__

#include "mx_digital_input.h"

typedef struct {
	MX_RECORD *record;

	MX_RECORD *k8055_record;
	unsigned long channel;

} MX_K8055_DINPUT;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_k8055_dinput_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxd_k8055_dinput_read( MX_DIGITAL_INPUT *dinput );

extern MX_RECORD_FUNCTION_LIST mxd_k8055_dinput_record_function_list;
extern MX_DIGITAL_INPUT_FUNCTION_LIST
			mxd_k8055_dinput_digital_input_function_list;

extern long mxd_k8055_dinput_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_k8055_dinput_rfield_def_ptr;

#define MXD_K8055_DINPUT_STANDARD_FIELDS \
  {-1, -1, "k8055_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_K8055_DINPUT, k8055_record),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "channel", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_K8055_DINPUT, channel), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

#endif /* __D_K8055_DINPUT_H__ */

