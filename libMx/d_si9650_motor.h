/*
 * Name:    d_si9650_motor.h
 *
 * Purpose: Header file for MX motor driver to control a Scientific Instruments
 *          9650 temperature controller as if it were a motor.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2003 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_SI9650_MOTOR_H__
#define __D_SI9650_MOTOR_H__

/* ===== Scientific Instruments 9650 motor data structures ===== */

typedef struct {
	MX_MOTOR *motor;	/* Used by mxd_si9650_motor_command() */

	MX_INTERFACE controller_interface;
	double maximum_ramp_rate;	/* in K per minute */
	double busy_deadband;		/* in K */

	int sensor_number;
} MX_SI9650_MOTOR;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_si9650_motor_create_record_structures(
					MX_RECORD *record );
MX_API mx_status_type mxd_si9650_motor_finish_record_initialization(
					MX_RECORD *record );
MX_API mx_status_type mxd_si9650_motor_print_motor_structure(
					FILE *file, MX_RECORD *record );
MX_API mx_status_type mxd_si9650_motor_open( MX_RECORD *record );

MX_API mx_status_type mxd_si9650_motor_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_si9650_motor_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_si9650_motor_get_extended_status( MX_MOTOR *motor );

MX_API mx_status_type mxd_si9650_motor_command(
				MX_SI9650_MOTOR *si9650_motor,
				char *command,
				char *response, size_t max_response_length,
				int debug_flag );

extern MX_RECORD_FUNCTION_LIST mxd_si9650_motor_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_si9650_motor_motor_function_list;

extern long mxd_si9650_motor_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_si9650_motor_rfield_def_ptr;

#define MXD_SI9650_MOTOR_STANDARD_FIELDS \
  {-1, -1, "controller_interface", MXFT_INTERFACE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SI9650_MOTOR, controller_interface), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "maximum_ramp_rate", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SI9650_MOTOR, maximum_ramp_rate), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "busy_deadband", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SI9650_MOTOR, busy_deadband), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __D_SI9650_MOTOR_H__ */
