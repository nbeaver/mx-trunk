/*
 * Name:    d_epics_motor.h 
 *
 * Purpose: Header file for MX motor driver for the EPICS APS-XFD developed
 *          motor record.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2003-2006, 2013 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_EPICS_MOTOR_H__
#define __D_EPICS_MOTOR_H__

/* ===== MX EPICS motor record data structures ===== */

typedef struct {
	char epics_record_name[ MXU_EPICS_PVNAME_LENGTH+1 ];
	double epics_record_version;
	int driver_type;

	MX_EPICS_PV accl_pv;
	MX_EPICS_PV dcof_pv;
	MX_EPICS_PV dir_pv;
	MX_EPICS_PV dmov_pv;
	MX_EPICS_PV hls_pv;
	MX_EPICS_PV homf_pv;
	MX_EPICS_PV homr_pv;
	MX_EPICS_PV icof_pv;
	MX_EPICS_PV jogf_pv;
	MX_EPICS_PV jogr_pv;
	MX_EPICS_PV lls_pv;
	MX_EPICS_PV lvio_pv;
	MX_EPICS_PV miss_pv;
	MX_EPICS_PV msta_pv;
	MX_EPICS_PV pcof_pv;
	MX_EPICS_PV rbv_pv;
	MX_EPICS_PV set_pv;
	MX_EPICS_PV spmg_pv;
	MX_EPICS_PV stop_pv;
	MX_EPICS_PV val_pv;
	MX_EPICS_PV vbas_pv;
	MX_EPICS_PV velo_pv;
} MX_EPICS_MOTOR;

/* Values for the 'driver_type' field. */

#define MXT_EPICS_MOTOR_UNKNOWN		0
#define MXT_EPICS_MOTOR_BCDA		1
#define MXT_EPICS_MOTOR_TIEMAN		2
#define MXT_EPICS_MOTOR_MX		3

/* Define all of the interface functions. */

MX_API mx_status_type mxd_epics_motor_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_epics_motor_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_epics_motor_print_structure(
					FILE *file, MX_RECORD *record );

MX_API mx_status_type mxd_epics_motor_motor_is_busy( MX_MOTOR *motor );
MX_API mx_status_type mxd_epics_motor_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_epics_motor_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_epics_motor_set_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_epics_motor_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_epics_motor_immediate_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_epics_motor_positive_limit_hit( MX_MOTOR *motor );
MX_API mx_status_type mxd_epics_motor_negative_limit_hit( MX_MOTOR *motor );
MX_API mx_status_type mxd_epics_motor_raw_home_command( MX_MOTOR *motor );
MX_API mx_status_type mxd_epics_motor_constant_velocity_move( MX_MOTOR *motor );
MX_API mx_status_type mxd_epics_motor_get_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_epics_motor_set_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_epics_motor_simultaneous_start(long num_motor_records,
						MX_RECORD **motor_record_array,
						double *position_array,
						unsigned long flags );
MX_API mx_status_type mxd_epics_motor_get_status( MX_MOTOR *motor );
MX_API mx_status_type mxd_epics_motor_get_extended_status( MX_MOTOR *motor );

extern MX_RECORD_FUNCTION_LIST mxd_epics_motor_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_epics_motor_motor_function_list;

extern long mxd_epics_motor_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_epics_motor_rfield_def_ptr;

#define MXD_EPICS_MOTOR_STANDARD_FIELDS \
  {-1, -1, "epics_record_name", MXFT_STRING, \
		NULL, 1, {MXU_EPICS_PVNAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EPICS_MOTOR, epics_record_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __D_EPICS_MOTOR_H__ */

