/*
 * Name:    d_phidget_old_stepper.h 
 *
 * Purpose: Header file for MX motor driver to control old non-HID Phidget
 *          stepper motor controllers via libusb on Linux and MacOS X.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2004 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_PHIDGET_OLD_STEPPER_H__
#define __D_PHIDGET_OLD_STEPPER_H__

/* ===== Phidget old stepper motor controller data structure ===== */

typedef struct {
	MX_RECORD *record;

	MX_RECORD *controller_record;
	int motor_number;

	double default_speed;
	double default_base_speed;
	double default_acceleration;
} MX_PHIDGET_OLD_STEPPER;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_phidget_old_stepper_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_phidget_old_stepper_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_phidget_old_stepper_print_structure(
					FILE *file, MX_RECORD *record );
MX_API mx_status_type mxd_phidget_old_stepper_open( MX_RECORD *record );
MX_API mx_status_type mxd_phidget_old_stepper_resynchronize(
					MX_RECORD *record );

MX_API mx_status_type mxd_phidget_old_stepper_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_phidget_old_stepper_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_phidget_old_stepper_get_extended_status(
							MX_MOTOR *motor );

extern MX_RECORD_FUNCTION_LIST mxd_phidget_old_stepper_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_phidget_old_stepper_motor_function_list;

extern long mxd_phidget_old_stepper_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_phidget_old_stepper_rfield_def_ptr;

#define MXD_PHIDGET_OLD_STEPPER_STANDARD_FIELDS \
  {-1, -1, "controller_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PHIDGET_OLD_STEPPER, controller_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "motor_number", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PHIDGET_OLD_STEPPER, motor_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "default_speed", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PHIDGET_OLD_STEPPER, default_speed), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "default_base_speed", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PHIDGET_OLD_STEPPER, default_base_speed), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "default_acceleration", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PHIDGET_OLD_STEPPER, \
					default_acceleration), \
	{0}, NULL, MXFF_IN_DESCRIPTION}

#endif /* __D_PHIDGET_OLD_STEPPER_H__ */
