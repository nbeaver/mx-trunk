/*
 * Name:    d_itc503_motor.h
 *
 * Purpose: Header file for MX motor driver to control an Oxford Instruments
 *          ITC503 temperature controller as if it were a motor.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2003 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_ITC503_MOTOR_H__
#define __D_ITC503_MOTOR_H__

/* Values for 'itc503_motor_flags' below */

#define MXF_ITC503_ENABLE_REMOTE_MODE	0x1
#define MXF_ITC503_UNLOCK		0x2

/* The two lowest order bits in 'itc503_motor_flags' are used to
 * construct a 'Cn' control command.  The 'Cn' determines whether
 * or not the controller is in LOCAL or REMOTE mode and also
 * whether or not the LOC/REM button is locked or active.  The
 * possible values for the 'Cn' command are:
 * 
 * C0 - Local and locked (default state)
 * C1 - Remote and locked (front panel disabled)
 * C2 - Local and unlocked
 * C3 - Remote and unlocked (front panel disabled)
 */

/* ===== ITC503 motor data structures ===== */

typedef struct {
	MX_RECORD *record;

	MX_INTERFACE controller_interface;
	int isobus_address;

	unsigned long itc503_motor_flags;

	int maximum_retries;

	double busy_deadband;		/* in K */
} MX_ITC503_MOTOR;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_itc503_motor_create_record_structures(
					MX_RECORD *record );
MX_API mx_status_type mxd_itc503_motor_finish_record_initialization(
					MX_RECORD *record );
MX_API mx_status_type mxd_itc503_motor_delete_record( MX_RECORD *record );
MX_API mx_status_type mxd_itc503_motor_print_motor_structure(
					FILE *file, MX_RECORD *record );
MX_API mx_status_type mxd_itc503_motor_open( MX_RECORD *record );

MX_API mx_status_type mxd_itc503_motor_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_itc503_motor_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_itc503_motor_get_extended_status( MX_MOTOR *motor );

MX_API mx_status_type mxd_itc503_motor_command(
				MX_ITC503_MOTOR *itc503_motor,
				char *command,
				char *response, size_t max_response_length,
				int debug_flag );

extern MX_RECORD_FUNCTION_LIST mxd_itc503_motor_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_itc503_motor_motor_function_list;

extern long mxd_itc503_motor_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_itc503_motor_rfield_def_ptr;

#define MXD_ITC503_MOTOR_STANDARD_FIELDS \
  {-1, -1, "controller_interface", MXFT_INTERFACE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ITC503_MOTOR, controller_interface), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "isobus_address", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ITC503_MOTOR, isobus_address), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "itc503_motor_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ITC503_MOTOR, itc503_motor_flags), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "maximum_retries", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ITC503_MOTOR, maximum_retries), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "busy_deadband", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ITC503_MOTOR, busy_deadband), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __D_ITC503_MOTOR_H__ */
