/*
 * Name:    d_auto_amplifier.h
 *
 * Purpose: Header file for autoscaling of amplifier settings.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2001 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef _D_AUTO_AMPLIFIER_H_
#define _D_AUTO_AMPLIFIER_H_

typedef struct {
	double present_amplifier_gain;

	double gain_range[MX_AMPLIFIER_NUM_GAIN_RANGE_PARAMS];
} MX_AUTO_AMPLIFIER;

#define MX_AUTO_AMPLIFIER_STANDARD_FIELDS \
  {-1, -1, "present_amplifier_gain", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_AUTO_AMPLIFIER, present_amplifier_gain), \
	{0}, NULL, 0 }

MX_API_PRIVATE mx_status_type mxd_auto_amplifier_initialize_type( long );
MX_API_PRIVATE mx_status_type mxd_auto_amplifier_create_record_structures(
							MX_RECORD *record );
MX_API_PRIVATE mx_status_type mxd_auto_amplifier_finish_record_initialization(
							MX_RECORD *record );
MX_API_PRIVATE mx_status_type mxd_auto_amplifier_delete_record(
							MX_RECORD *record );
MX_API_PRIVATE mx_status_type mxd_auto_amplifier_open( MX_RECORD *record );
MX_API_PRIVATE mx_status_type mxd_auto_amplifier_dummy_function(
							MX_RECORD *record );

MX_API_PRIVATE mx_status_type mxd_auto_amplifier_read_monitor(
						MX_AUTOSCALE *autoscale );

MX_API_PRIVATE mx_status_type mxd_auto_amplifier_get_change_request(
						MX_AUTOSCALE *autoscale );

MX_API_PRIVATE mx_status_type mxd_auto_amplifier_change_control(
						MX_AUTOSCALE *autoscale );

MX_API_PRIVATE mx_status_type mxd_auto_amplifier_get_offset_index(
						MX_AUTOSCALE *autoscale );

MX_API_PRIVATE mx_status_type mxd_auto_amplifier_set_offset_index(
						MX_AUTOSCALE *autoscale );

MX_API_PRIVATE mx_status_type mxd_auto_amplifier_get_parameter(
						MX_AUTOSCALE *autoscale );

MX_API_PRIVATE mx_status_type mxd_auto_amplifier_set_parameter(
						MX_AUTOSCALE *autoscale );

extern MX_RECORD_FUNCTION_LIST mxd_auto_amplifier_record_function_list;
extern MX_AUTOSCALE_FUNCTION_LIST mxd_auto_amplifier_autoscale_function_list;

extern long mxd_auto_amplifier_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_auto_amplifier_rfield_def_ptr;

#endif /* _D_AUTO_AMPLIFIER_H_ */

