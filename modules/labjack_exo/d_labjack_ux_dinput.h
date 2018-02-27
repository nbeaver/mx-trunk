/*
 * Name:    d_labjack_ux_dinput.h
 *
 * Purpose: Header file for LabJack U3, U6, and UE9 digital input pins.
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

#ifndef __D_LABJACK_UX_DINPUT_H__
#define __D_LABJACK_UX_DINPUT_H__

#include "mx_digital_input.h"

typedef struct {
	MX_RECORD *record;

	MX_RECORD *labjack_ux_record;
	unsigned long pin_number;

} MX_LABJACK_UX_DINPUT;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_labjack_ux_dinput_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_labjack_ux_dinput_finish_record_initialization(
							MX_RECORD *record );

MX_API mx_status_type mxd_labjack_ux_dinput_read( MX_DIGITAL_INPUT *dinput );

extern MX_RECORD_FUNCTION_LIST mxd_labjack_ux_dinput_record_function_list;
extern MX_DIGITAL_INPUT_FUNCTION_LIST
			mxd_labjack_ux_dinput_digital_input_function_list;

extern long mxd_labjack_ux_dinput_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_labjack_ux_dinput_rfield_def_ptr;

#define MXD_LABJACK_UX_DINPUT_STANDARD_FIELDS \
  {-1, -1, "labjack_ux_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_LABJACK_UX_DINPUT, labjack_ux_record),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "pin_number", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_LABJACK_UX_DINPUT, pin_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

#endif /* __D_LABJACK_UX_DINPUT_H__ */

