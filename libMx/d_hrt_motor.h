/*
 * Name:    d_hrt_motor.h 
 *
 * Purpose: Header file for MX motor driver that allows one to scan using
 *          the MX high resolution timer as the independent variable.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2022 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_HRT_MOTOR_H__
#define __D_HRT_MOTOR_H__

#include "mx_motor.h"

typedef struct {
	double time_of_last_reset;      /* in seconds */
	double cpu_speed;		/* in Hz */
} MX_HRT_MOTOR;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_hrt_motor_create_record_structures(
					MX_RECORD *record );
MX_API mx_status_type mxd_hrt_motor_special_processing_setup(
					MX_RECORD *record );

MX_API mx_status_type mxd_hrt_motor_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_hrt_motor_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_hrt_motor_set_position( MX_MOTOR *motor );

extern MX_RECORD_FUNCTION_LIST mxd_hrt_motor_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_hrt_motor_motor_function_list;

extern long mxd_hrt_motor_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_hrt_motor_rfield_def_ptr;

#define MXLV_HRT_MTR_CPU_SPEED		43901

#define MXD_HRT_MOTOR_STANDARD_FIELDS \
  {-1, -1, "time_of_last_reset", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_HRT_MOTOR, time_of_last_reset), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {MXLV_HRT_MTR_CPU_SPEED, -1, "cpu_speed", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_HRT_MOTOR, cpu_speed ), \
	{0}, NULL, 0}

#endif /* __D_HRT_MOTOR_H__ */

