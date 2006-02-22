/*
 * Name:    d_pcmotion32.h
 *
 * Purpose: Header file for MX motor driver for the National Instruments
 *          PCMOTION32.DLL used with the ValueMotion series of motor
 *          controllers originally sold by nuLogic.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2000-2001 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_PCMOTION32_H__
#define __D_PCMOTION32_H__

#include "i_pcmotion32.h"

/* ============ Motor channels ============ */

typedef struct {
	MX_RECORD *controller_record;
	int axis_id;

	unsigned long default_speed;
	unsigned long default_base_speed;
	unsigned long default_acceleration;
	unsigned long default_acceleration_factor;

	unsigned short lines_per_revolution;
	unsigned short steps_per_revolution;
} MX_PCMOTION32_MOTOR;

MX_API mx_status_type mxd_pcmotion32_initialize_type( long type );
MX_API mx_status_type mxd_pcmotion32_create_record_structures(
						MX_RECORD *record );
MX_API mx_status_type mxd_pcmotion32_finish_record_initialization(
						MX_RECORD *record );
MX_API mx_status_type mxd_pcmotion32_delete_record( MX_RECORD *record );
MX_API mx_status_type mxd_pcmotion32_print_structure( FILE *file,
						MX_RECORD *record );
MX_API mx_status_type mxd_pcmotion32_read_parms_from_hardware(
						MX_RECORD *record );
MX_API mx_status_type mxd_pcmotion32_write_parms_to_hardware(
						MX_RECORD *record );
MX_API mx_status_type mxd_pcmotion32_open( MX_RECORD *record );
MX_API mx_status_type mxd_pcmotion32_close( MX_RECORD *record );
MX_API mx_status_type mxd_pcmotion32_resynchronize( MX_RECORD *record );

MX_API mx_status_type mxd_pcmotion32_motor_is_busy( MX_MOTOR *motor );
MX_API mx_status_type mxd_pcmotion32_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_pcmotion32_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_pcmotion32_set_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_pcmotion32_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_pcmotion32_immediate_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_pcmotion32_positive_limit_hit( MX_MOTOR *motor );
MX_API mx_status_type mxd_pcmotion32_negative_limit_hit( MX_MOTOR *motor );
MX_API mx_status_type mxd_pcmotion32_find_home_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_pcmotion32_constant_velocity_move( MX_MOTOR *motor );
MX_API mx_status_type mxd_pcmotion32_get_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_pcmotion32_set_parameter( MX_MOTOR *motor );

extern MX_RECORD_FUNCTION_LIST mxd_pcmotion32_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_pcmotion32_motor_function_list;

extern long mxd_pcmotion32_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_pcmotion32_rfield_def_ptr;

#define MXD_PCMOTION32_MOTOR_STANDARD_FIELDS \
  {-1, -1, "controller_record", MXFT_RECORD, NULL, 0, {0}, \
    MXF_REC_TYPE_STRUCT, offsetof(MX_PCMOTION32_MOTOR, controller_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "axis_id", MXFT_INT, NULL, 0, {0}, \
    MXF_REC_TYPE_STRUCT, offsetof(MX_PCMOTION32_MOTOR, axis_id), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "default_speed", MXFT_ULONG, NULL, 0, {0}, \
    MXF_REC_TYPE_STRUCT, offsetof(MX_PCMOTION32_MOTOR, default_speed), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "default_base_speed", MXFT_ULONG, NULL, 0, {0}, \
    MXF_REC_TYPE_STRUCT, offsetof(MX_PCMOTION32_MOTOR, default_base_speed), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "default_acceleration", MXFT_ULONG, NULL, 0, {0}, \
    MXF_REC_TYPE_STRUCT, offsetof(MX_PCMOTION32_MOTOR, default_acceleration), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "default_acceleration_factor", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCMOTION32_MOTOR, \
					default_acceleration_factor), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "lines_per_revolution", MXFT_USHORT, NULL, 0, {0}, \
    MXF_REC_TYPE_STRUCT, offsetof(MX_PCMOTION32_MOTOR, lines_per_revolution), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "steps_per_revolution", MXFT_USHORT, NULL, 0, {0}, \
    MXF_REC_TYPE_STRUCT, offsetof(MX_PCMOTION32_MOTOR, steps_per_revolution), \
	{0}, NULL, MXFF_IN_DESCRIPTION}

#endif /* __D_PCMOTION32_H__ */
