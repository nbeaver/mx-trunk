/*
 * Name:    d_qmotor.h 
 *
 * Purpose: Header file for MX pseudomotor driver for the momentum transfer
 *          parameter Q.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2000, 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_QMOTOR_H__
#define __D_QMOTOR_H__

#include "mx_motor.h"

/* ===== MX q motor data structures ===== */

typedef struct {
	MX_RECORD *theta_motor_record;
	MX_RECORD *wavelength_motor_record;
	double angle_scale; /* To translate betw. radians and native units. */
} MX_Q_MOTOR;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_q_motor_create_record_structures(
					MX_RECORD *record );
MX_API mx_status_type mxd_q_motor_finish_record_initialization(
					MX_RECORD *record );
MX_API mx_status_type mxd_q_motor_print_motor_structure(
					FILE *file, MX_RECORD *record );

MX_API mx_status_type mxd_q_motor_motor_is_busy( MX_MOTOR *motor );
MX_API mx_status_type mxd_q_motor_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_q_motor_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_q_motor_set_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_q_motor_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_q_motor_immediate_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_q_motor_positive_limit_hit( MX_MOTOR *motor );
MX_API mx_status_type mxd_q_motor_negative_limit_hit( MX_MOTOR *motor );
MX_API mx_status_type mxd_q_motor_find_home_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_q_motor_constant_velocity_move(MX_MOTOR *motor);
MX_API mx_status_type mxd_q_motor_get_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_q_motor_set_parameter( MX_MOTOR *motor );

extern MX_RECORD_FUNCTION_LIST mxd_q_motor_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_q_motor_motor_function_list;

extern long mxd_q_motor_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_q_motor_rfield_def_ptr;

#define MXD_Q_MOTOR_STANDARD_FIELDS \
  {-1, -1, "theta_motor_record", MXFT_RECORD, NULL, 0, {0}, \
        MXF_REC_TYPE_STRUCT, offsetof(MX_Q_MOTOR, theta_motor_record ), \
        {0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "wavelength_motor_record", MXFT_RECORD, NULL, 0, {0}, \
        MXF_REC_TYPE_STRUCT, offsetof(MX_Q_MOTOR, wavelength_motor_record),\
        {0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "angle_scale", MXFT_DOUBLE, NULL, 0, {0}, \
        MXF_REC_TYPE_STRUCT, offsetof(MX_Q_MOTOR, angle_scale),\
        {0}, NULL, MXFF_IN_DESCRIPTION}

#endif /* __D_QMOTOR_H__ */
