/*
 * Name:    d_energy.h 
 *
 * Purpose: Header file for MX motor driver to control an MX motor device
 *          in units of X-ray energy.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2004, 2013 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_ENERGY_H__
#define __D_ENERGY_H__

#include "mx_motor.h"
#include "mx_constants.h"

/* ===== MX energy motor data structures ===== */

/* The angle scale is used to translate between radians and the
 * native units of the theta axis.  Usually the theta motor will
 * be using degrees as its native units.  In that case, the correct
 * value for angle_scale should pi/180 = 0.01745329251994330,
 * but the record allows for the possibility that theta will 
 * not be using degrees.
 */

typedef struct {
	MX_RECORD *dependent_motor_record;
	MX_RECORD *d_spacing_record;
	double angle_scale;

	double saved_d_spacing;
} MX_ENERGY_MOTOR;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_energy_motor_create_record_structures(
					MX_RECORD *record );
MX_API mx_status_type mxd_energy_motor_finish_record_initialization(
					MX_RECORD *record );
MX_API mx_status_type mxd_energy_motor_print_motor_structure(
					FILE *file, MX_RECORD *record );
MX_API mx_status_type mxd_energy_motor_open( MX_RECORD *record );

MX_API mx_status_type mxd_energy_motor_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_energy_motor_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_energy_motor_set_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_energy_motor_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_energy_motor_immediate_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_energy_motor_raw_home_command( MX_MOTOR *motor );
MX_API mx_status_type mxd_energy_motor_constant_velocity_move(MX_MOTOR *motor);
MX_API mx_status_type mxd_energy_motor_get_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_energy_motor_set_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_energy_motor_get_status( MX_MOTOR *motor );
MX_API mx_status_type mxd_energy_motor_get_extended_status( MX_MOTOR *motor );

extern MX_RECORD_FUNCTION_LIST mxd_energy_motor_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_energy_motor_motor_function_list;

extern long mxd_energy_motor_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_energy_motor_rfield_def_ptr;

#define MXD_ENERGY_MOTOR_STANDARD_FIELDS \
  {-1, -1, "dependent_motor_record", MXFT_RECORD, NULL, 0, {0}, \
        MXF_REC_TYPE_STRUCT, \
                offsetof(MX_ENERGY_MOTOR, dependent_motor_record ), \
        {0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "d_spacing_record", MXFT_RECORD, NULL, 0, {0}, \
        MXF_REC_TYPE_STRUCT, offsetof(MX_ENERGY_MOTOR, d_spacing_record),\
        {0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "angle_scale", MXFT_DOUBLE, NULL, 0, {0}, \
        MXF_REC_TYPE_STRUCT, offsetof(MX_ENERGY_MOTOR, angle_scale),\
        {0}, NULL, MXFF_IN_DESCRIPTION}

#endif /* __D_ENERGY_H__ */
