/*
 * Name:    d_pmac.h
 *
 * Purpose: Header file for MX drivers for Delta Tau PMAC controlled motors.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999-2003, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_PMAC_H__
#define __D_PMAC_H__

#include "i_pmac.h"

/* ============ Motor channels ============ */

typedef struct {
	MX_RECORD *record;	/* Pointer to the MX record that contains
				 * a record_type_struct pointer to this
				 * structure.
				 */
	MX_RECORD *pmac_record;
	int card_number;
	int motor_number;
} MX_PMAC_MOTOR;

MX_API mx_status_type mxd_pmac_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxd_pmac_finish_record_initialization(MX_RECORD *record);
MX_API mx_status_type mxd_pmac_print_structure( FILE *file, MX_RECORD *record );
MX_API mx_status_type mxd_pmac_close( MX_RECORD *record );
MX_API mx_status_type mxd_pmac_resynchronize( MX_RECORD *record );

MX_API mx_status_type mxd_pmac_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_pmac_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_pmac_set_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_pmac_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_pmac_immediate_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_pmac_find_home_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_pmac_constant_velocity_move( MX_MOTOR *motor );
MX_API mx_status_type mxd_pmac_get_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_pmac_set_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_pmac_simultaneous_start( int num_motor_records,
						MX_RECORD **motor_record_array,
						double *position_array,
						int flags );
MX_API mx_status_type mxd_pmac_get_status( MX_MOTOR *motor );

extern MX_RECORD_FUNCTION_LIST mxd_pmac_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_pmac_motor_function_list;

extern mx_length_type mxd_pmac_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_pmac_rfield_def_ptr;

#define MXD_PMAC_STANDARD_FIELDS \
  {-1, -1, "pmac_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PMAC_MOTOR, pmac_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "card_number", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PMAC_MOTOR, card_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "motor_number", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PMAC_MOTOR, motor_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/* === Driver specific functions === */

MX_API mx_status_type mxd_pmac_jog_command( MX_PMAC_MOTOR *pmac_motor,
						char *command,
						char *response,
						int response_buffer_length,
						int debug_flag );

MX_API mx_status_type mxd_pmac_get_motor_variable( MX_PMAC_MOTOR *pmac_motor,
						int variable_number,
						int variable_type,
						void *variable_ptr,
						int debug_flag );

MX_API mx_status_type mxd_pmac_set_motor_variable( MX_PMAC_MOTOR *pmac_motor,
						int variable_number,
						int variable_type,
						void *variable_ptr,
						int debug_flag );

#endif /* __D_PMAC_H__ */

