/*
 * Name:    d_cubic_spline_motor.h 
 *
 * Purpose: MX pseudomotor driver that uses a cubic spline to compute the
 *          position of a dependent motor.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_CUBIC_SPLINE_MOTOR_H__
#define __D_CUBIC_SPLINE_MOTOR_H__

#include "mx_motor.h"
#include "mx_math.h"

typedef struct {
	MX_RECORD *dependent_motor_record;
	MX_RECORD *x_array_record;
	MX_RECORD *y_array_record;

	MX_CUBIC_SPLINE *spline;
} MX_CUBIC_SPLINE_MOTOR;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_cubic_spline_motor_create_record_structures(
					MX_RECORD *record );
MX_API mx_status_type mxd_cubic_spline_motor_finish_record_initialization(
					MX_RECORD *record );
MX_API mx_status_type mxd_cubic_spline_motor_delete_record( MX_RECORD *record );
MX_API mx_status_type mxd_cubic_spline_motor_open( MX_RECORD *record );

MX_API mx_status_type mxd_cubic_spline_motor_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_cubic_spline_motor_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_cubic_spline_motor_immediate_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_cubic_spline_motor_get_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_cubic_spline_motor_set_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_cubic_spline_motor_get_status( MX_MOTOR *motor );
MX_API mx_status_type mxd_cubic_spline_motor_get_extended_status(
							MX_MOTOR *motor );

extern MX_RECORD_FUNCTION_LIST mxd_cubic_spline_motor_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_cubic_spline_motor_motor_function_list;

extern long mxd_cubic_spline_motor_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_cubic_spline_motor_rfield_def_ptr;

#define MXD_CUBIC_SPLINE_MOTOR_STANDARD_FIELDS \
  {-1, -1, "dependent_motor_record", MXFT_RECORD, NULL, 0, {0}, \
        MXF_REC_TYPE_STRUCT, \
                offsetof(MX_CUBIC_SPLINE_MOTOR, dependent_motor_record ), \
        {0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "x_array_record", MXFT_RECORD, NULL, 0, {0}, \
        MXF_REC_TYPE_STRUCT, offsetof(MX_CUBIC_SPLINE_MOTOR, x_array_record ), \
        {0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "y_array_record", MXFT_RECORD, NULL, 0, {0}, \
        MXF_REC_TYPE_STRUCT, offsetof(MX_CUBIC_SPLINE_MOTOR, y_array_record ), \
        {0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

#endif /* __D_CUBIC_SPLINE_MOTOR_H__ */
