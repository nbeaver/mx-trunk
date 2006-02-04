/*
 * Name:    d_dacmtr.h 
 *
 * Purpose: Header file for MX motor driver to control an MX analog output
 *          device as if it were a motor.  The most common use of this is
 *          to control piezoelectric motors.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_DAC_MOTOR_H__
#define __D_DAC_MOTOR_H__

#include "mx_motor.h"

/* ===== MX DAC motor data structures ===== */

typedef struct {
	MX_RECORD *dac_record;
} MX_DAC_MOTOR;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_dac_motor_create_record_structures(
					MX_RECORD *record );

MX_API mx_status_type mxd_dac_motor_motor_is_busy( MX_MOTOR *motor );
MX_API mx_status_type mxd_dac_motor_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_dac_motor_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_dac_motor_set_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_dac_motor_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_dac_motor_immediate_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_dac_motor_positive_limit_hit( MX_MOTOR *motor );
MX_API mx_status_type mxd_dac_motor_negative_limit_hit( MX_MOTOR *motor );

extern MX_RECORD_FUNCTION_LIST mxd_dac_motor_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_dac_motor_motor_function_list;

extern mx_length_type mxd_dac_motor_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_dac_motor_rfield_def_ptr;

#define MXD_DAC_MOTOR_STANDARD_FIELDS \
  {-1, -1, "dac_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DAC_MOTOR, dac_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __D_DAC_MOTOR_H__ */
