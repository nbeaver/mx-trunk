/*
 * Name:    d_ggcs.h
 *
 * Purpose: Header file for MX drivers for Bruker GGCS controlled motors.
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

#ifndef __D_GGCS_H__
#define __D_GGCS_H__

#include "i_ggcs.h"

/* ============ Motor channels ============ */

typedef struct {
	MX_RECORD *ggcs_record;
	int32_t axis_number;
} MX_GGCS_MOTOR;

MX_API mx_status_type mxd_ggcs_motor_initialize_type( long type );
MX_API mx_status_type mxd_ggcs_motor_create_record_structures(
						MX_RECORD *record );
MX_API mx_status_type mxd_ggcs_motor_finish_record_initialization(
						MX_RECORD *record );
MX_API mx_status_type mxd_ggcs_motor_delete_record( MX_RECORD *record );
MX_API mx_status_type mxd_ggcs_motor_print_structure( FILE *file,
						MX_RECORD *record );
MX_API mx_status_type mxd_ggcs_motor_read_parms_from_hardware(
						MX_RECORD *record );
MX_API mx_status_type mxd_ggcs_motor_write_parms_to_hardware(
						MX_RECORD *record );
MX_API mx_status_type mxd_ggcs_motor_open( MX_RECORD *record );
MX_API mx_status_type mxd_ggcs_motor_close( MX_RECORD *record );

MX_API mx_status_type mxd_ggcs_motor_motor_is_busy( MX_MOTOR *motor );
MX_API mx_status_type mxd_ggcs_motor_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_ggcs_motor_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_ggcs_motor_set_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_ggcs_motor_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_ggcs_motor_immediate_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_ggcs_motor_positive_limit_hit( MX_MOTOR *motor );
MX_API mx_status_type mxd_ggcs_motor_negative_limit_hit( MX_MOTOR *motor );
MX_API mx_status_type mxd_ggcs_motor_find_home_position( MX_MOTOR *motor );

extern MX_RECORD_FUNCTION_LIST mxd_ggcs_motor_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_ggcs_motor_motor_function_list;

extern mx_length_type mxd_ggcs_motor_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_ggcs_motor_rfield_def_ptr;

#define MXD_GGCS_MOTOR_STANDARD_FIELDS \
  {-1, -1, "ggcs_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_GGCS_MOTOR, ggcs_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "axis_number", MXFT_INT32, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_GGCS_MOTOR, axis_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

#endif /* __D_GGCS_H__ */
