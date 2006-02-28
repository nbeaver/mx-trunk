/*
 * Name:    d_cryostream600_motor.h
 *
 * Purpose: Header file for MX motor driver to control a Cryostream 600 series
 *          temperature controller as if it were a motor.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2002 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_CRYOSTREAM600_MOTOR_H__
#define __D_CRYOSTREAM600_MOTOR_H__

/* ===== Cryostream 600 motor data structures ===== */

typedef struct {
	MX_MOTOR *motor;	/* Used by mxd_cryostream600_motor_command() */

	MX_RECORD *rs232_record;
	double ramp_rate;		/* in K per hour */
	double busy_deadband;		/* in K */

	long max_response_tokens;
	long max_token_length;
	char **response_token_array;

	long num_tokens_returned;
} MX_CRYOSTREAM600_MOTOR;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_cryostream600_motor_create_record_structures(
					MX_RECORD *record );
MX_API mx_status_type mxd_cryostream600_motor_finish_record_initialization(
					MX_RECORD *record );
MX_API mx_status_type mxd_cryostream600_motor_delete_record( MX_RECORD *record );
MX_API mx_status_type mxd_cryostream600_motor_print_motor_structure(
					FILE *file, MX_RECORD *record );
MX_API mx_status_type mxd_cryostream600_motor_open( MX_RECORD *record );

MX_API mx_status_type mxd_cryostream600_motor_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_cryostream600_motor_positive_limit_hit(
							MX_MOTOR *motor );
MX_API mx_status_type mxd_cryostream600_motor_negative_limit_hit(
							MX_MOTOR *motor );
MX_API mx_status_type mxd_cryostream600_motor_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_cryostream600_motor_get_extended_status(
							MX_MOTOR *motor );

MX_API mx_status_type mxd_cryostream600_motor_command(
				MX_CRYOSTREAM600_MOTOR *cryostream600_motor,
				char *command,
				char *response, size_t max_response_length,
				int debug_flag );

MX_API mx_status_type mxd_cryostream600_motor_parse_response(
				MX_CRYOSTREAM600_MOTOR *cryostream600_motor,
				char *response, int debug_flag );

extern MX_RECORD_FUNCTION_LIST mxd_cryostream600_motor_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_cryostream600_motor_motor_function_list;

extern long mxd_cryostream600_motor_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_cryostream600_motor_rfield_def_ptr;

#define MXD_CRYOSTREAM600_MOTOR_STANDARD_FIELDS \
  {-1, -1, "rs232_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_CRYOSTREAM600_MOTOR, rs232_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "ramp_rate", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_CRYOSTREAM600_MOTOR, ramp_rate), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "busy_deadband", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_CRYOSTREAM600_MOTOR, busy_deadband), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __D_CRYOSTREAM600_MOTOR_H__ */
