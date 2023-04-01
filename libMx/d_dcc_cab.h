/*
 * Name:    d_dcc_cab.h 
 *
 * Purpose: Header file for MX motor driver for a DCC-controlled
 *          model railroad locomotive.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2023 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_DCC_CAB_H__
#define __D_DCC_CAB_H__

/* ===== Pontech DCC_CAB data structure ===== */

typedef struct {
	MX_RECORD *record;

	MX_RECORD *rs232_record;
	long cab_number;
	long packet_register;
	char encoder_record_name[ MXU_RECORD_NAME_LENGTH+1 ];

	MX_RECORD *encoder_record;
} MX_DCC_CAB;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_dcc_cab_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_dcc_cab_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_dcc_cab_open( MX_RECORD *record );

MX_API mx_status_type mxd_dcc_cab_is_busy( MX_MOTOR *motor );
MX_API mx_status_type mxd_dcc_cab_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_dcc_cab_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_dcc_cab_set_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_dcc_cab_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_dcc_cab_immediate_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_dcc_cab_constant_velocity_move( MX_MOTOR *motor);
MX_API mx_status_type mxd_dcc_cab_get_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_dcc_cab_set_parameter( MX_MOTOR *motor );

extern MX_RECORD_FUNCTION_LIST mxd_dcc_cab_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_dcc_cab_motor_function_list;

/* Define some extra functions for the use of this driver. */

MX_API mx_status_type mxd_dcc_cab_command( MX_DCC_CAB *stp100_motor,
						char *command,
						char *response,
						size_t response_buffer_length,
						int debug_flag );

extern long mxd_dcc_cab_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_dcc_cab_rfield_def_ptr;

#define MXD_DCC_CAB_STANDARD_FIELDS \
  {-1, -1, "rs232_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DCC_CAB, rs232_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "cab_number", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DCC_CAB, cab_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "packet_register", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DCC_CAB, packet_register), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "encoder_record_name", MXFT_STRING, NULL, 1, \
	  					{MXU_RECORD_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DCC_CAB, encoder_record_name), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __D_DCC_CAB_H__ */

