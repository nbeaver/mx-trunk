/*
 * Name:    d_mclennan.h 
 *
 * Purpose: Header file for MX motor driver to control McLennan 
 *          motor controllers.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2002, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_MCLENNAN_H__
#define __D_MCLENNAN_H__

/* ===== McLennan motor data structures ===== */

#define MXT_MCLENNAN_UNKNOWN	0
#define MXT_MCLENNAN_PM301	301
#define MXT_MCLENNAN_PM304	304
#define MXT_MCLENNAN_PM341	341
#define MXT_MCLENNAN_PM381	381
#define MXT_MCLENNAN_PM600	600

typedef struct {
	MX_RECORD *record;

	MX_RECORD *rs232_record;
	long axis_number;
	long axis_encoder_number;

	long controller_type;

	long num_dinput_ports;
	long num_doutput_ports;
	long num_ainput_ports;
	long num_aoutput_ports;
} MX_MCLENNAN;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_mclennan_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxd_mclennan_finish_record_initialization(
					MX_RECORD *record );
MX_API mx_status_type mxd_mclennan_print_motor_structure(
					FILE *file, MX_RECORD *record );
MX_API mx_status_type mxd_mclennan_open( MX_RECORD *record );
MX_API mx_status_type mxd_mclennan_resynchronize( MX_RECORD *record );

MX_API mx_status_type mxd_mclennan_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_mclennan_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_mclennan_set_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_mclennan_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_mclennan_immediate_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_mclennan_find_home_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_mclennan_constant_velocity_move( MX_MOTOR *motor );
MX_API mx_status_type mxd_mclennan_get_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_mclennan_set_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_mclennan_get_status( MX_MOTOR *motor );

extern MX_RECORD_FUNCTION_LIST mxd_mclennan_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_mclennan_motor_function_list;

/* Define some extra functions for the use of this driver. */

MX_API mx_status_type mxd_mclennan_command( MX_MCLENNAN *mclennan,char *command,
			char *response, long response_buffer_length,
			int debug_flag );

extern long mxd_mclennan_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_mclennan_rfield_def_ptr;

#define MXD_MCLENNAN_STANDARD_FIELDS \
  {-1, -1, "rs232_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MCLENNAN, rs232_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "axis_number", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MCLENNAN, axis_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "axis_encoder_number", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MCLENNAN, axis_encoder_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "controller_type", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MCLENNAN, controller_type), \
	{0}, NULL, 0}

#endif /* __D_MCLENNAN_H__ */
