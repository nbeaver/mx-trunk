/*
 * Name:    d_panther.h 
 *
 * Purpose: Header file for MX motor driver to control Panther HI & HE
 *          motor controllers made by Intelligent Motion Systems, Inc.
 *          of Marlborough, Connecticut.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_PANTHER_H__
#define __D_PANTHER_H__

#include "mx_motor.h"

/* ===== Panther HI & HE motor data structures ===== */

typedef struct {
	MX_RECORD  *rs232_record;

	/* If axis_name == '0', then we are in Single Mode. */

	char axis_name;

	int32_t default_speed;			/* command 'V' */
	int32_t default_base_speed;		/* command 'I' */
	int32_t acceleration_slope;		/* command 'K' */
	int32_t deceleration_slope;		/* command 'K' */
	int32_t microstep_divide_factor;	/* command 'D' */
	char    step_resolution_mode;		/* command 'H' */
	int32_t hold_current;			/* command 'Y' */
	int32_t run_current;			/* command 'Y' */
	int32_t settling_time_delay;		/* command 'E' */
	int32_t limit_polarity;			/* command 'l' */

	int32_t encoder_resolution;		/* command 'e' */
	int32_t deadband_size;			/* command 'd' */
	int32_t hunt_velocity;			/* command 'v' */
	int32_t hunt_resolution;		/* command 'h' */
	int32_t stall_factor;			/* command 's' */
	int32_t stall_sample_rate;		/* command 't' */
	int32_t max_stall_retries;		/* command 'r' */
} MX_PANTHER;

/* Step resolution modes. */

#define MX_PANTHER_FIXED_RESOLUTION	'F'
#define MX_PANTHER_VARIABLE_RESOLUTION	'V'

/* End of command string characters for Party Line Mode or Single Mode. */

#ifndef MX_CR
#define MX_CR	'\015'
#define MX_LF	'\012'
#endif

#define MX_PANTHER_PARTY_LINE_MODE_EOS	MX_LF
#define MX_PANTHER_SINGLE_MODE_EOS	MX_CR

#define MX_PANTHER_MOTOR_MINIMUM_SPEED	20L
#define MX_PANTHER_MOTOR_MAXIMUM_SPEED	20000L

#define MX_PANTHER_MOTOR_MINIMUM_ACCEL_SLOPE	0L
#define MX_PANTHER_MOTOR_MAXIMUM_ACCEL_SLOPE	255L

/* Define all of the interface functions. */

MX_API mx_status_type mxd_panther_create_record_structures(MX_RECORD *record);
MX_API mx_status_type mxd_panther_finish_record_initialization(
							MX_RECORD *record);
MX_API mx_status_type mxd_panther_print_motor_structure(
					FILE *file, MX_RECORD *record );
MX_API mx_status_type mxd_panther_read_parms_from_hardware( MX_RECORD *record);
MX_API mx_status_type mxd_panther_write_parms_to_hardware( MX_RECORD *record );
MX_API mx_status_type mxd_panther_open( MX_RECORD *record );
MX_API mx_status_type mxd_panther_close( MX_RECORD *record );
MX_API mx_status_type mxd_panther_resynchronize( MX_RECORD *record );

MX_API mx_status_type mxd_panther_motor_is_busy( MX_MOTOR *motor );
MX_API mx_status_type mxd_panther_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_panther_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_panther_set_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_panther_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_panther_immediate_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_panther_positive_limit_hit( MX_MOTOR *motor );
MX_API mx_status_type mxd_panther_negative_limit_hit( MX_MOTOR *motor );
MX_API mx_status_type mxd_panther_find_home_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_panther_constant_velocity_move( MX_MOTOR *motor );
MX_API mx_status_type mxd_panther_get_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_panther_set_parameter( MX_MOTOR *motor );

extern MX_RECORD_FUNCTION_LIST mxd_panther_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_panther_motor_function_list;

/* Define some extra functions for the use of this driver. */

MX_API mx_status_type mxd_panther_getline( MX_PANTHER *panther,
			char *buffer, int buffer_length, int debug_flag );
MX_API mx_status_type mxd_panther_putline( MX_PANTHER *panther,
			char *buffer, int echo_eos_flag, int debug_flag );
MX_API mx_status_type mxd_panther_getc( MX_PANTHER *panther,
			char *c, int debug_flag );
MX_API mx_status_type mxd_panther_getc_nowait( MX_PANTHER *panther,
			char *c, int debug_flag );
MX_API mx_status_type mxd_panther_putc( MX_PANTHER *panther,
			char c, int debug_flag );

extern mx_length_type mxd_panther_hi_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_panther_hi_rfield_def_ptr;

extern mx_length_type mxd_panther_he_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_panther_he_rfield_def_ptr;

#define MXD_PANTHER_HI_STANDARD_FIELDS \
  {-1, -1, "rs232_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PANTHER, rs232_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "axis_name", MXFT_CHAR, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PANTHER, axis_name), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "default_speed", MXFT_INT32, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PANTHER, default_speed), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "default_base_speed", MXFT_INT32, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PANTHER, default_base_speed), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "acceleration_slope", MXFT_INT32, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PANTHER, acceleration_slope), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "deceleration_slope", MXFT_INT32, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PANTHER, deceleration_slope), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "microstep_divide_factor", MXFT_INT32, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PANTHER, microstep_divide_factor),\
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "step_resolution_mode", MXFT_CHAR, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PANTHER, step_resolution_mode), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "hold_current", MXFT_INT32, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PANTHER, hold_current), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "run_current", MXFT_INT32, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PANTHER, run_current), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "settling_time_delay", MXFT_INT32, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PANTHER, settling_time_delay), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "limit_polarity", MXFT_INT32, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PANTHER, limit_polarity), \
	{0}, NULL, MXFF_IN_DESCRIPTION}

#define MXD_PANTHER_HE_STANDARD_FIELDS \
  {-1, -1, "encoder_resolution", MXFT_INT32, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PANTHER, encoder_resolution), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "deadband_size", MXFT_INT32, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PANTHER, deadband_size), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "hunt_velocity", MXFT_INT32, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PANTHER, hunt_velocity), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "hunt_resolution", MXFT_INT32, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PANTHER, hunt_resolution), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "stall_factor", MXFT_INT32, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PANTHER, stall_factor), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "stall_sample_rate", MXFT_INT32, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PANTHER, stall_sample_rate), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "max_stall_retries", MXFT_INT32, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PANTHER, max_stall_retries), \
	{0}, NULL, MXFF_IN_DESCRIPTION}

#endif /* __D_PANTHER_H__ */
