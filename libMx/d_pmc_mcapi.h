/*
 * Name:    d_pmc_mcapi_motor.h
 *
 * Purpose: Header file for MX drivers for Precision MicroControl
 *          MCAPI controlled motors.
 *
 * Author:  William Lavender
 *
 *----------------------------------------------------------------------------
 *
 * Copyright 2004, 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_PMC_MCAPI_MOTOR_H__
#define __D_PMC_MCAPI_MOTOR_H__

typedef struct {
	MX_RECORD *record;

	MX_RECORD *pmc_mcapi_record;
	unsigned short axis_number;

#if defined(__MCAPI_H__) || defined(_INC_MCAPI)

	/* mcapi.h has been included. */

	MCAXISCONFIG configuration;
#endif
} MX_PMC_MCAPI_MOTOR;

MX_API mx_status_type mxd_pmc_mcapi_create_record_structures(
						MX_RECORD *record );
MX_API mx_status_type mxd_pmc_mcapi_finish_record_initialization(
						MX_RECORD *record );
MX_API mx_status_type mxd_pmc_mcapi_print_structure( FILE *file,
						MX_RECORD *record );
MX_API mx_status_type mxd_pmc_mcapi_open( MX_RECORD *record );
MX_API mx_status_type mxd_pmc_mcapi_resynchronize( MX_RECORD *record );

MX_API mx_status_type mxd_pmc_mcapi_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_pmc_mcapi_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_pmc_mcapi_set_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_pmc_mcapi_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_pmc_mcapi_immediate_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_pmc_mcapi_find_home_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_pmc_mcapi_constant_velocity_move( MX_MOTOR *motor );
MX_API mx_status_type mxd_pmc_mcapi_get_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_pmc_mcapi_set_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_pmc_mcapi_simultaneous_start( int num_motor_records,
						MX_RECORD **motor_record_array,
						double *position_array,
						int flags );
MX_API mx_status_type mxd_pmc_mcapi_get_status( MX_MOTOR *motor );

extern MX_RECORD_FUNCTION_LIST mxd_pmc_mcapi_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_pmc_mcapi_motor_function_list;

extern long mxd_pmc_mcapi_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_pmc_mcapi_rfield_def_ptr;

#define MXD_PMC_MCAPI_MOTOR_STANDARD_FIELDS \
  {-1, -1, "pmc_mcapi_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_PMC_MCAPI_MOTOR, pmc_mcapi_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "axis_number", MXFT_USHORT, NULL, 0, {0},\
	MXF_REC_TYPE_STRUCT, offsetof(MX_PMC_MCAPI_MOTOR, axis_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

#endif /* __D_PMC_MCAPI_MOTOR_H__ */
