/*
 * Name:    d_newport.h
 *
 * Purpose: Header file for MX drivers for the Newport MM3000, MM4000/4005,
 *          and ESP series of motor controllers.
 *
 * Author:  William Lavender
 *
 *-----------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2003, 2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_NEWPORT_H__
#define __D_NEWPORT_H__

#include "i_newport.h"

/* ============ Motor channels ============ */

typedef struct {
	MX_RECORD *newport_record;
	int axis_number;

	int hardware_limits_active_high;
} MX_NEWPORT_MOTOR;

MX_API mx_status_type mxd_newport_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxd_newport_finish_record_initialization(
						MX_RECORD *record );
MX_API mx_status_type mxd_newport_print_structure( FILE *file,
						MX_RECORD *record );
MX_API mx_status_type mxd_newport_open( MX_RECORD *record );
MX_API mx_status_type mxd_newport_resynchronize( MX_RECORD *record );

MX_API mx_status_type mxd_newport_motor_is_busy( MX_MOTOR *motor );
MX_API mx_status_type mxd_newport_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_newport_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_newport_set_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_newport_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_newport_immediate_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_newport_positive_limit_hit( MX_MOTOR *motor );
MX_API mx_status_type mxd_newport_negative_limit_hit( MX_MOTOR *motor );
MX_API mx_status_type mxd_newport_find_home_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_newport_constant_velocity_move( MX_MOTOR *motor );
MX_API mx_status_type mxd_newport_get_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_newport_set_parameter( MX_MOTOR *motor );

extern MX_RECORD_FUNCTION_LIST mxd_newport_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_newport_motor_function_list;

extern long mxd_mm3000_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_mm3000_rfield_def_ptr;

extern long mxd_mm4000_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_mm4000_rfield_def_ptr;

extern long mxd_esp_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_esp_rfield_def_ptr;

#define MXD_NEWPORT_STANDARD_FIELDS \
  {-1, -1, "newport_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NEWPORT_MOTOR, newport_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "axis_number", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NEWPORT_MOTOR, axis_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

#endif /* __D_NEWPORT_H__ */
