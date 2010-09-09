/*
 * Name:    d_auto_filter.h
 *
 * Purpose: Header file for autoscaling of filter settings.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2001, 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef _D_AUTO_FILTER_H_
#define _D_AUTO_FILTER_H_

typedef struct {
	unsigned long present_filter_setting;

	unsigned long minimum_filter_setting;
	unsigned long maximum_filter_setting;
} MX_AUTO_FILTER;

#define MX_AUTO_FILTER_STANDARD_FIELDS \
  {-1, -1, "minimum_filter_setting", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AUTO_FILTER, minimum_filter_setting),\
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "maximum_filter_setting", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AUTO_FILTER, maximum_filter_setting),\
	{0}, NULL, MXFF_IN_DESCRIPTION }

MX_API_PRIVATE mx_status_type mxd_auto_filter_create_record_structures(
							MX_RECORD *record );
MX_API_PRIVATE mx_status_type mxd_auto_filter_finish_record_initialization(
							MX_RECORD *record );
MX_API_PRIVATE mx_status_type mxd_auto_filter_delete_record(
							MX_RECORD *record );
MX_API_PRIVATE mx_status_type mxd_auto_filter_open( MX_RECORD *record );

MX_API_PRIVATE mx_status_type mxd_auto_filter_read_monitor(
						MX_AUTOSCALE *autoscale );

MX_API_PRIVATE mx_status_type mxd_auto_filter_get_change_request(
						MX_AUTOSCALE *autoscale );

MX_API_PRIVATE mx_status_type mxd_auto_filter_change_control(
						MX_AUTOSCALE *autoscale );

MX_API_PRIVATE mx_status_type mxd_auto_filter_get_offset_index(
						MX_AUTOSCALE *autoscale );

MX_API_PRIVATE mx_status_type mxd_auto_filter_set_offset_index(
						MX_AUTOSCALE *autoscale );

MX_API_PRIVATE mx_status_type mxd_auto_filter_get_parameter(
						MX_AUTOSCALE *autoscale );

MX_API_PRIVATE mx_status_type mxd_auto_filter_set_parameter(
						MX_AUTOSCALE *autoscale );

extern MX_RECORD_FUNCTION_LIST mxd_auto_filter_record_function_list;
extern MX_AUTOSCALE_FUNCTION_LIST mxd_auto_filter_autoscale_function_list;

extern long mxd_auto_filter_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_auto_filter_rfield_def_ptr;

#endif /* _D_AUTO_FILTER_H_ */

