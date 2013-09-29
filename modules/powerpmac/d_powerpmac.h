/*
 * Name:    d_powerpmac.h
 *
 * Purpose: Header file for MX drivers for Delta Tau PowerPMAC
 *          controlled motors.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2010, 2012-2013 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_POWERPMAC_H__
#define __D_POWERPMAC_H__

#include "i_powerpmac.h"

/* ============ Motor channels ============ */

typedef struct {
	MX_RECORD *record;
	MX_RECORD *powerpmac_record;
	long motor_number;

	struct MotorData *motor_data;

	mx_bool_type use_shm;
} MX_POWERPMAC_MOTOR;

MX_API mx_status_type mxd_powerpmac_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_powerpmac_finish_record_initialization(
							MX_RECORD *record);
MX_API mx_status_type mxd_powerpmac_open( MX_RECORD *record );

MX_API mx_status_type mxd_powerpmac_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_powerpmac_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_powerpmac_set_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_powerpmac_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_powerpmac_immediate_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_powerpmac_raw_home_command( MX_MOTOR *motor );
MX_API mx_status_type mxd_powerpmac_constant_velocity_move( MX_MOTOR *motor );
MX_API mx_status_type mxd_powerpmac_get_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_powerpmac_set_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_powerpmac_simultaneous_start( long num_motor_records,
						MX_RECORD **motor_record_array,
						double *position_array,
						unsigned long flags );
MX_API mx_status_type mxd_powerpmac_get_status( MX_MOTOR *motor );

extern MX_RECORD_FUNCTION_LIST mxd_powerpmac_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_powerpmac_motor_function_list;

extern long mxd_powerpmac_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_powerpmac_rfield_def_ptr;

#define MXD_POWERPMAC_STANDARD_FIELDS \
  {-1, -1, "powerpmac_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_POWERPMAC_MOTOR, powerpmac_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "motor_number", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_POWERPMAC_MOTOR, motor_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/* === Driver specific functions === */

MX_API mx_status_type mxd_powerpmac_jog_command(
					MX_POWERPMAC_MOTOR *powerpmac_motor,
					MX_POWERPMAC *powerpmac,
					char *command,
					char *response,
					size_t response_buffer_length,
					int debug_flag );

MX_API mx_status_type mxd_powerpmac_get_motor_variable(
					MX_POWERPMAC_MOTOR *powerpmac_motor,
					MX_POWERPMAC *powerpmac,
					char *variable_name,
					long variable_type,
					double *variable_value,
					int debug_flag );

MX_API mx_status_type mxd_powerpmac_set_motor_variable(
					MX_POWERPMAC_MOTOR *powerpmac_motor,
					MX_POWERPMAC *powerpmac,
					char *variable_name,
					long variable_type,
					double variable_value,
					int debug_flag );

#endif /* __D_POWERPMAC_H__ */

