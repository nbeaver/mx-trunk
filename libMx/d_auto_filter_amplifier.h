/*
 * Name:    d_auto_filter_amplifier.h
 *
 * Purpose: Header file for autoscaling of filter settings with an associated
 *          autoscaling of an amplifier.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2001, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef _D_AUTO_FILTER_AMPLIFIER_H_
#define _D_AUTO_FILTER_AMPLIFIER_H_

typedef struct {
	uint32_t present_filter_setting;

	uint32_t minimum_filter_setting;
	uint32_t maximum_filter_setting;

	MX_RECORD *amplifier_autoscale_record;
} MX_AUTO_FILTER_AMP;

#define MX_AUTO_FILTER_AMP_STANDARD_FIELDS \
  {-1, -1, "minimum_filter_setting", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_AUTO_FILTER_AMP, minimum_filter_setting),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "maximum_filter_setting", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_AUTO_FILTER_AMP, maximum_filter_setting),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "amplifier_autoscale_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_AUTO_FILTER_AMP, amplifier_autoscale_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

MX_API_PRIVATE mx_status_type mxd_auto_filter_amp_initialize_type( long );
MX_API_PRIVATE mx_status_type mxd_auto_filter_amp_create_record_structures(
							MX_RECORD *record );
MX_API_PRIVATE mx_status_type mxd_auto_filter_amp_finish_record_initialization(
							MX_RECORD *record );
MX_API_PRIVATE mx_status_type mxd_auto_filter_amp_delete_record(
							MX_RECORD *record );
MX_API_PRIVATE mx_status_type mxd_auto_filter_amp_open( MX_RECORD *record );
MX_API_PRIVATE mx_status_type mxd_auto_filter_amp_dummy_function(
							MX_RECORD *record );

MX_API_PRIVATE mx_status_type mxd_auto_filter_amp_read_monitor(
						MX_AUTOSCALE *autoscale );

MX_API_PRIVATE mx_status_type mxd_auto_filter_amp_get_change_request(
						MX_AUTOSCALE *autoscale );

MX_API_PRIVATE mx_status_type mxd_auto_filter_amp_change_control(
						MX_AUTOSCALE *autoscale );

MX_API_PRIVATE mx_status_type mxd_auto_filter_amp_get_offset_index(
						MX_AUTOSCALE *autoscale );

MX_API_PRIVATE mx_status_type mxd_auto_filter_amp_set_offset_index(
						MX_AUTOSCALE *autoscale );

MX_API_PRIVATE mx_status_type mxd_auto_filter_amp_get_parameter(
						MX_AUTOSCALE *autoscale );

MX_API_PRIVATE mx_status_type mxd_auto_filter_amp_set_parameter(
						MX_AUTOSCALE *autoscale );

extern MX_RECORD_FUNCTION_LIST mxd_auto_filter_amp_record_function_list;
extern MX_AUTOSCALE_FUNCTION_LIST mxd_auto_filter_amp_autoscale_function_list;

extern mx_length_type mxd_auto_filter_amp_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_auto_filter_amp_rfield_def_ptr;

#endif /* _D_AUTO_FILTER_AMPLIFIER_H_ */

