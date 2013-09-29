/*
 * Name:    d_mcu2.h
 *
 * Purpose: Header file for the Advanced Control Systems MCU-2 motor controller.
 *
 * Author:  William Lavender
 *
 *----------------------------------------------------------------------------
 *
 * Copyright 2005-2006, 2013 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_MCU2_H__
#define __D_MCU2_H__

/* Values for the mcu2_flags variable. */

#define MXF_MCU2_HOME_TO_LIMIT_SWITCH	0x1
#define MXF_MCU2_NO_START_CHARACTER	0x2

typedef struct {
	MX_RECORD *record;

	MX_RECORD *rs232_record;
	long axis_address;
	unsigned long mcu2_flags;
} MX_MCU2;

MX_API mx_status_type mxd_mcu2_create_record_structures(
						MX_RECORD *record );
MX_API mx_status_type mxd_mcu2_finish_record_initialization(
						MX_RECORD *record );
MX_API mx_status_type mxd_mcu2_print_structure( FILE *file,
						MX_RECORD *record );
MX_API mx_status_type mxd_mcu2_open( MX_RECORD *record );

MX_API mx_status_type mxd_mcu2_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_mcu2_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_mcu2_set_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_mcu2_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_mcu2_immediate_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_mcu2_raw_home_command( MX_MOTOR *motor );
MX_API mx_status_type mxd_mcu2_constant_velocity_move( MX_MOTOR *motor );
MX_API mx_status_type mxd_mcu2_get_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_mcu2_set_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_mcu2_get_status( MX_MOTOR *motor );

extern MX_RECORD_FUNCTION_LIST mxd_mcu2_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_mcu2_motor_function_list;

extern long mxd_mcu2_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_mcu2_rfield_def_ptr;

#define MXD_MCU2_STANDARD_FIELDS \
  {-1, -1, "rs232_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_MCU2, rs232_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "axis_address", MXFT_LONG, NULL, 0, {0},\
	MXF_REC_TYPE_STRUCT, offsetof(MX_MCU2, axis_address), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "mcu2_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MCU2, mcu2_flags), \
	{0}, NULL, MXFF_IN_DESCRIPTION }

#endif /* __D_MCU2_H__ */
