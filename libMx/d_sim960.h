/*
 * Name:    d_sim960.h
 *
 * Purpose: Header file for the Stanford Research Systems SIM960
 *          Analog PID Controller.
 *
 * Author:  William Lavender
 *
 *----------------------------------------------------------------------------
 *
 * Copyright 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_SIM960_H__
#define __D_SIM960_H__

/* Values for the sim960_flags variable. */

#define MXF_SIM960_EXTERNAL_SETPOINT	0x1

typedef struct {
	MX_RECORD *record;

	MX_RECORD *port_record;
	unsigned long sim960_flags;

	double version;
} MX_SIM960;

MX_API mx_status_type mxd_sim960_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxd_sim960_open( MX_RECORD *record );

MX_API mx_status_type mxd_sim960_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_sim960_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_sim960_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_sim960_get_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_sim960_set_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_sim960_get_status( MX_MOTOR *motor );

extern MX_RECORD_FUNCTION_LIST mxd_sim960_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_sim960_motor_function_list;

extern long mxd_sim960_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_sim960_rfield_def_ptr;

#define MXD_SIM960_STANDARD_FIELDS \
  {-1, -1, "port_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SIM960, port_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "sim960_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SIM960, sim960_flags), \
	{0}, NULL, MXFF_IN_DESCRIPTION }

#endif /* __D_SIM960_H__ */
