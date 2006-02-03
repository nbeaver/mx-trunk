/*
 * Name:    d_scipe_motor.h 
 *
 * Purpose: Header file for the MX motor driver for SCIPE controlled motors.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2000-2002, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_SCIPE_MOTOR_H__
#define __D_SCIPE_MOTOR_H__

#include "mx_motor.h"

/* ===== MX SCIPE motor record data structures ===== */

typedef struct {
	MX_RECORD *scipe_server_record;

	char scipe_motor_name[MX_SCIPE_OBJECT_NAME_LENGTH+1];
} MX_SCIPE_MOTOR;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_scipe_motor_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_scipe_motor_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_scipe_motor_print_structure(
					FILE *file, MX_RECORD *record );
MX_API mx_status_type mxd_scipe_motor_open( MX_RECORD *record );
MX_API mx_status_type mxd_scipe_motor_resynchronize( MX_RECORD *record );

MX_API mx_status_type mxd_scipe_motor_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_scipe_motor_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_scipe_motor_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_scipe_motor_get_status( MX_MOTOR *motor );

extern MX_RECORD_FUNCTION_LIST mxd_scipe_motor_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_scipe_motor_motor_function_list;

extern mx_length_type mxd_scipe_motor_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_scipe_motor_rfield_def_ptr;

#define MXD_SCIPE_MOTOR_STANDARD_FIELDS \
  {-1, -1, "scipe_server_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SCIPE_MOTOR, scipe_server_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "scipe_motor_name", MXFT_STRING, NULL, \
					1, {MX_SCIPE_OBJECT_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SCIPE_MOTOR, scipe_motor_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __D_SCIPE_MOTOR_H__ */

