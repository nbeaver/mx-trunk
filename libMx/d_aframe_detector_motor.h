/*
 * Name:    d_aframe_detector_motor.h 
 *
 * Purpose: Header file for MX motor driver for the pseudomotors used by
 *          Gerd Rosenbaum's A-frame CCD detector mount.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2002, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_AFRAME_DETECTOR_MOTOR_H__
#define __D_AFRAME_DETECTOR_MOTOR_H__

/* The following are the supported values for detector_motor_type. */

#define MXT_AFRAME_DETECTOR_DISTANCE		1
#define MXT_AFRAME_DETECTOR_HORIZONTAL_ANGLE	2
#define MXT_AFRAME_DETECTOR_OFFSET		3

typedef struct {
	MX_RECORD *record;

	int32_t pseudomotor_type;

	MX_RECORD *A_record;
	MX_RECORD *B_record;
	MX_RECORD *C_record;

	MX_RECORD *dv_downstream_record;
	MX_RECORD *dv_upstream_record;
	MX_RECORD *dh_record;

	double A_value;
	double B_value;
	double C_value;
	double dv_downstream_position;
	double dv_upstream_position;
	double dh_position;
} MX_AFRAME_DETECTOR_MOTOR;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_aframe_det_motor_initialize_type( long type );
MX_API mx_status_type mxd_aframe_det_motor_create_record_structures(
					MX_RECORD *record );
MX_API mx_status_type mxd_aframe_det_motor_finish_record_initialization(
					MX_RECORD *record );
MX_API mx_status_type mxd_aframe_det_motor_delete_record( MX_RECORD *record );
MX_API mx_status_type mxd_aframe_det_motor_print_motor_structure(
					FILE *file, MX_RECORD *record );
MX_API mx_status_type mxd_aframe_det_motor_read_parms_from_hardware(
					MX_RECORD *record );
MX_API mx_status_type mxd_aframe_det_motor_write_parms_to_hardware(
					MX_RECORD *record );
MX_API mx_status_type mxd_aframe_det_motor_open( MX_RECORD *record );
MX_API mx_status_type mxd_aframe_det_motor_close( MX_RECORD *record );
MX_API mx_status_type mxd_aframe_det_motor_resynchronize( MX_RECORD *record );
MX_API mx_status_type mxd_aframe_det_motor_is_busy( MX_MOTOR *motor );
MX_API mx_status_type mxd_aframe_det_motor_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_aframe_det_motor_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_aframe_det_motor_set_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_aframe_det_motor_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_aframe_det_motor_immediate_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_aframe_det_motor_positive_limit_hit(
							MX_MOTOR *motor );
MX_API mx_status_type mxd_aframe_det_motor_negative_limit_hit(
							MX_MOTOR *motor );
MX_API mx_status_type mxd_aframe_det_motor_get_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_aframe_det_motor_set_parameter( MX_MOTOR *motor );

extern MX_RECORD_FUNCTION_LIST mxd_aframe_det_motor_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_aframe_det_motor_motor_function_list;

extern mx_length_type mxd_aframe_det_motor_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_aframe_det_motor_rfield_def_ptr;

#define MXD_AFRAME_DETECTOR_MOTOR_STANDARD_FIELDS \
  {-1, -1, "pseudomotor_type", MXFT_INT32, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_AFRAME_DETECTOR_MOTOR, pseudomotor_type),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "A_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AFRAME_DETECTOR_MOTOR, A_record), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "B_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AFRAME_DETECTOR_MOTOR, B_record), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "C_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AFRAME_DETECTOR_MOTOR, C_record), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "dv_downstream_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_AFRAME_DETECTOR_MOTOR, dv_downstream_record), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "dv_upstream_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_AFRAME_DETECTOR_MOTOR, dv_upstream_record), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "dh_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AFRAME_DETECTOR_MOTOR, dh_record), \
	{0}, NULL, MXFF_IN_DESCRIPTION }

#endif /* __D_AFRAME_DETECTOR_MOTOR_H__ */
