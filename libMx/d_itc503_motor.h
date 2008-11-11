/*
 * Name:    d_itc503_motor.h
 *
 * Purpose: Header file for MX motor driver to control Oxford Instruments
 *          ITC503 and Cryojet temperature controllers as if they were motors.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2003, 2006, 2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_ITC503_MOTOR_H__
#define __D_ITC503_MOTOR_H__

/* ===== ITC503 motor data structures ===== */

typedef struct {
	MX_RECORD *record;

	MX_RECORD *itc503_record;

	double busy_deadband;		/* in K */
} MX_ITC503_MOTOR;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_itc503_motor_create_record_structures(
					MX_RECORD *record );
MX_API mx_status_type mxd_itc503_motor_print_motor_structure(
					FILE *file, MX_RECORD *record );

MX_API mx_status_type mxd_itc503_motor_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_itc503_motor_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_itc503_motor_get_extended_status( MX_MOTOR *motor );

extern MX_RECORD_FUNCTION_LIST mxd_itc503_motor_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_itc503_motor_motor_function_list;

extern long mxd_itc503_motor_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_itc503_motor_rfield_def_ptr;

#define MXD_ITC503_MOTOR_STANDARD_FIELDS \
  {-1, -1, "itc503_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ITC503_MOTOR, itc503_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "busy_deadband", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ITC503_MOTOR, busy_deadband), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __D_ITC503_MOTOR_H__ */

