/*
 * Name:    d_slit_motor.h 
 *
 * Purpose: Header file for MX motor driver to use a pair of MX motors as
 *          the sides of a slit.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001-2002, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_SLIT_MOTOR_H__
#define __D_SLIT_MOTOR_H__

#include "mx_motor.h"

/* ===== MX slit motor data structures ===== */

typedef struct {
	mx_hex_type slit_flags;

	int32_t slit_type;

	MX_RECORD *negative_motor_record;
	MX_RECORD *positive_motor_record;
	double negative_motor_position;
	double positive_motor_position;

	double pseudomotor_start_position;
	double saved_negative_motor_start_position;
	double saved_positive_motor_start_position;
} MX_SLIT_MOTOR;

/* Values for the slit_flags field. */

#define MXF_SLIT_MOTOR_SIMULTANEOUS_START	0x1

/* The MXF_SLIT_... defines below are used as the value for the
 * 'slit_type' field in the record description.
 *
 * The SAME case below is for slit blade pairs that move in the same
 * physical direction when they are both commanded to perform moves
 * of the same sign.  The OPPOSITE case is for slit blade pairs that 
 * move in the opposite direction from each other when they are both
 * commanded to perform moves of the same sign.
 *
 * As an example, for a top and bottom slit blade pair, if a positive
 * move of each causes both blades to go up, you use the SAME case.
 * On the other hand, if a positive move of each causes the top slit
 * blade to go up and the bottom slit blade to go down, you use the
 * OPPOSITE case.
 */

#define MXF_SLIT_CENTER_SAME		1
#define MXF_SLIT_WIDTH_SAME		2
#define MXF_SLIT_CENTER_OPPOSITE	3
#define MXF_SLIT_WIDTH_OPPOSITE		4

/* Define all of the interface functions. */

MX_API mx_status_type mxd_slit_motor_create_record_structures(
					MX_RECORD *record );
MX_API mx_status_type mxd_slit_motor_finish_record_initialization(
					MX_RECORD *record );
MX_API mx_status_type mxd_slit_motor_print_motor_structure(
					FILE *file, MX_RECORD *record );

MX_API mx_status_type mxd_slit_motor_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_slit_motor_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_slit_motor_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_slit_motor_immediate_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_slit_motor_set_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_slit_motor_get_status( MX_MOTOR *motor );

extern MX_RECORD_FUNCTION_LIST mxd_slit_motor_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_slit_motor_motor_function_list;

extern mx_length_type mxd_slit_motor_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_slit_motor_rfield_def_ptr;

#define MXD_SLIT_MOTOR_STANDARD_FIELDS \
  {-1, -1, "slit_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SLIT_MOTOR, slit_flags), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "slit_type", MXFT_INT32, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SLIT_MOTOR, slit_type), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "negative_motor_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SLIT_MOTOR, negative_motor_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "positive_motor_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SLIT_MOTOR, positive_motor_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

#endif /* __D_SLIT_MOTOR_H__ */
