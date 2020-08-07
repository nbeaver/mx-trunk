/*
 * Name:    d_zwo_efw_motor.h
 *
 * Purpose: Header file for ZWO Electronic Filter Wheels treated as motors.
 *
 * Author:  William Lavender
 *
 *----------------------------------------------------------------------------
 *
 * Copyright 2020 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_ZWO_EFW_MOTOR_H__
#define __D_ZWO_EFW_MOTOR_H__

/*---*/

/* Values for 'motor_type'. */

typedef struct {
	MX_RECORD *record;

	MX_RECORD *zwo_efw_record;
	unsigned long filter_index;

	unsigned long filter_id;
	unsigned long num_filters;

	mx_bool_type home_search_succeeded;

#if defined(EFW_FILTER_H)
	EFW_INFO efw_info;
#endif

} MX_ZWO_EFW_MOTOR;

MX_API mx_status_type mxd_zwo_efw_motor_create_record_structures(
						MX_RECORD *record );
MX_API mx_status_type mxd_zwo_efw_motor_open( MX_RECORD *record );
MX_API mx_status_type mxd_zwo_efw_motor_close( MX_RECORD *record );
MX_API mx_status_type mxd_zwo_efw_motor_resynchronize( MX_RECORD *record );

MX_API mx_status_type mxd_zwo_efw_motor_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_zwo_efw_motor_raw_home_command( MX_MOTOR *motor );
MX_API mx_status_type mxd_zwo_efw_motor_get_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_zwo_efw_motor_set_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_zwo_efw_motor_get_extended_status(
						MX_MOTOR *motor );

extern MX_RECORD_FUNCTION_LIST mxd_zwo_efw_motor_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_zwo_efw_motor_motor_function_list;

extern long mxd_zwo_efw_motor_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_zwo_efw_motor_rfield_def_ptr;

#define MXD_ZWO_EFW_MOTOR_STANDARD_FIELDS \
  {-1, -1, "zwo_efw_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ZWO_EFW_MOTOR, zwo_efw_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "filter_index", MXFT_ULONG, NULL, 0, {0},\
	MXF_REC_TYPE_STRUCT, offsetof(MX_ZWO_EFW_MOTOR, filter_index), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_READ_ONLY) }, \
  \
  {-1, -1, "filter_id", MXFT_ULONG, NULL, 0, {0},\
	MXF_REC_TYPE_STRUCT, offsetof(MX_ZWO_EFW_MOTOR, filter_id), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "num_filters", MXFT_ULONG, NULL, 0, {0},\
	MXF_REC_TYPE_STRUCT, offsetof(MX_ZWO_EFW_MOTOR, num_filters), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "home_search_succeeded", MXFT_BOOL, NULL, 0, {0},\
	MXF_REC_TYPE_STRUCT, offsetof(MX_ZWO_EFW_MOTOR, home_search_succeeded),\
	{0}, NULL, MXFF_READ_ONLY }

#endif /* __D_ZWO_EFW_MOTOR_H__ */
