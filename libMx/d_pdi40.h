/*
 * Name:    d_pdi40.h
 *
 * Purpose: Header file for MX drivers for the PDI Model 40 DAQ interface.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999, 2001 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_PDI40_H__
#define __D_PDI40_H__

#include "i_pdi40.h"

/* ============ Motor channels ============ */

typedef struct {
	MX_RECORD *pdi40_record;
	char stepper_name;
} MX_PDI40_MOTOR;

#define MXD_PDI40_MOTOR_STANDARD_FIELDS \
  {-1, -1, "pdi40_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PDI40_MOTOR, pdi40_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "stepper_name", MXFT_CHAR, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PDI40_MOTOR, stepper_name), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

MX_API mx_status_type mxd_pdi40motor_initialize_type( long type );
MX_API mx_status_type mxd_pdi40motor_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_pdi40motor_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_pdi40motor_delete_record( MX_RECORD *record );
MX_API mx_status_type mxd_pdi40motor_read_parms_from_hardware(
						MX_RECORD *record );
MX_API mx_status_type mxd_pdi40motor_write_parms_to_hardware(
						MX_RECORD *record );
MX_API mx_status_type mxd_pdi40motor_open( MX_RECORD *record );
MX_API mx_status_type mxd_pdi40motor_close( MX_RECORD *record );


MX_API mx_status_type mxd_pdi40motor_motor_is_busy( MX_MOTOR *motor );
MX_API mx_status_type mxd_pdi40motor_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_pdi40motor_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_pdi40motor_set_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_pdi40motor_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_pdi40motor_immediate_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_pdi40motor_positive_limit_hit( MX_MOTOR *motor );
MX_API mx_status_type mxd_pdi40motor_negative_limit_hit( MX_MOTOR *motor );
MX_API mx_status_type mxd_pdi40motor_find_home_position( MX_MOTOR *motor );

extern MX_RECORD_FUNCTION_LIST mxd_pdi40motor_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_pdi40motor_motor_function_list;

extern long mxd_pdi40motor_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_pdi40motor_rfield_def_ptr;

#endif /* __D_PDI40_H__ */
