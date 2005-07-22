/*
 * Name:    d_smc24.h 
 *
 * Purpose: Header file for MX motor driver to control SMC24 CAMAC stepping 
 *          motor controllers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2004 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_SMC24_H__
#define __D_SMC24_H__

#include "mx_motor.h"

/* ===== SMC24 motor data structures ===== */

typedef struct {
	MX_RECORD *crate_record;
	int slot;
	MX_RECORD *encoder_record;
	double motor_steps_per_encoder_tick;
	int flags;
} MX_SMC24;

/* Bit masks for the "flags" field. */

#define MXF_SMC24_USE_32BIT_SOFTWARE_COUNTER	1
#define MXF_SMC24_USE_CW_CCW_MOTOR_PULSES	2

/* Define all of the interface functions. */

MX_API mx_status_type mxd_smc24_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxd_smc24_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_smc24_print_motor_structure(
					FILE *file, MX_RECORD *record );

MX_API mx_status_type mxd_smc24_motor_is_busy( MX_MOTOR *motor );
MX_API mx_status_type mxd_smc24_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_smc24_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_smc24_set_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_smc24_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_smc24_immediate_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_smc24_positive_limit_hit( MX_MOTOR *motor );
MX_API mx_status_type mxd_smc24_negative_limit_hit( MX_MOTOR *motor );
MX_API mx_status_type mxd_smc24_find_home_position( MX_MOTOR *motor );

extern MX_RECORD_FUNCTION_LIST mxd_smc24_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_smc24_motor_function_list;

extern long mxd_smc24_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_smc24_rfield_def_ptr;

#define MXD_SMC24_STANDARD_FIELDS \
  {-1, -1, "crate_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_SMC24, crate_record ), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "slot", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_SMC24, slot ), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "encoder_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_SMC24, encoder_record ), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "motor_steps_per_encoder_tick", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SMC24, motor_steps_per_encoder_tick),\
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "flags", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_SMC24, flags ), \
	{0}, NULL, MXFF_IN_DESCRIPTION}

#endif /* __D_SMC24_H__ */
