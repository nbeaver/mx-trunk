/*
 * Name:    d_pmac_cs_axis.h
 *
 * Purpose: Header file for MX driver for an axis in a coordinate system
 *          of a Delta Tau PMAC motor controller.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2003, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_PMAC_CS_AXIS_H__
#define __D_PMAC_CS_AXIS_H__

#include "i_pmac.h"

/* ============ Motor channels ============ */

typedef struct {
	MX_RECORD *record;

	MX_RECORD *pmac_record;
	int card_number;
	int coordinate_system;
	char axis_name;
	int move_program_number;
	char position_variable[ MXU_PMAC_VARIABLE_NAME_LENGTH + 1 ];
	char destination_variable[ MXU_PMAC_VARIABLE_NAME_LENGTH + 1 ];
	char feedrate_variable[ MXU_PMAC_VARIABLE_NAME_LENGTH + 1 ];
	char acceleration_time_variable[ MXU_PMAC_VARIABLE_NAME_LENGTH + 1 ];
	char s_curve_acceleration_time_var[ MXU_PMAC_VARIABLE_NAME_LENGTH + 1];

	double feedrate_time_unit_in_seconds;

} MX_PMAC_COORDINATE_SYSTEM_AXIS;

MX_API mx_status_type mxd_pmac_cs_axis_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_pmac_cs_axis_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_pmac_cs_axis_print_structure( FILE *file,
							MX_RECORD *record );
MX_API mx_status_type mxd_pmac_cs_axis_resynchronize( MX_RECORD *record );

MX_API mx_status_type mxd_pmac_cs_axis_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_pmac_cs_axis_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_pmac_cs_axis_set_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_pmac_cs_axis_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_pmac_cs_axis_immediate_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_pmac_cs_axis_get_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_pmac_cs_axis_set_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_pmac_cs_axis_simultaneous_start(
						int num_motor_records,
						MX_RECORD **motor_record_array,
						double *position_array,
						int flags );
MX_API mx_status_type mxd_pmac_cs_axis_get_status( MX_MOTOR *motor );

extern MX_RECORD_FUNCTION_LIST mxd_pmac_cs_axis_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_pmac_cs_axis_motor_function_list;

extern long mxd_pmac_cs_axis_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_pmac_cs_axis_rfield_def_ptr;

#define MXD_PMAC_CS_AXIS_STANDARD_FIELDS \
  {-1, -1, "pmac_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_PMAC_COORDINATE_SYSTEM_AXIS, pmac_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "card_number", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_PMAC_COORDINATE_SYSTEM_AXIS, card_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "coordinate_system", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_PMAC_COORDINATE_SYSTEM_AXIS, coordinate_system), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "axis_name", MXFT_CHAR, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_PMAC_COORDINATE_SYSTEM_AXIS, axis_name), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "move_program_number", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_PMAC_COORDINATE_SYSTEM_AXIS, move_program_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "position_variable", MXFT_STRING, \
	  			NULL, 1, {MXU_PMAC_VARIABLE_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
	   offsetof(MX_PMAC_COORDINATE_SYSTEM_AXIS, position_variable), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "destination_variable", MXFT_STRING, \
	  			NULL, 1, {MXU_PMAC_VARIABLE_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
	   offsetof(MX_PMAC_COORDINATE_SYSTEM_AXIS, destination_variable),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "feedrate_variable", MXFT_STRING, \
	  			NULL, 1, {MXU_PMAC_VARIABLE_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
	   offsetof(MX_PMAC_COORDINATE_SYSTEM_AXIS, feedrate_variable),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "acceleration_time_variable", MXFT_STRING, \
	  			NULL, 1, {MXU_PMAC_VARIABLE_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
   offsetof(MX_PMAC_COORDINATE_SYSTEM_AXIS, acceleration_time_variable),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "s_curve_acceleration_time_var", MXFT_STRING, \
	  			NULL, 1, {MXU_PMAC_VARIABLE_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
   offsetof(MX_PMAC_COORDINATE_SYSTEM_AXIS, s_curve_acceleration_time_var),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/* === Driver specific functions === */

MX_API mx_status_type mxd_pmac_cs_axis_command(
					MX_PMAC_COORDINATE_SYSTEM_AXIS *axis,
					MX_PMAC *pmac,
					char *command,
					char *response,
					int response_buffer_length,
					int debug_flag );

MX_API mx_status_type mxd_pmac_cs_axis_get_variable(
					MX_PMAC_COORDINATE_SYSTEM_AXIS *axis,
					MX_PMAC *pmac,
					char *variable_name,
					int variable_type,
					void *variable_ptr,
					int debug_flag );

MX_API mx_status_type mxd_pmac_cs_axis_set_variable(
					MX_PMAC_COORDINATE_SYSTEM_AXIS *axis,
					MX_PMAC *pmac,
					char *variable_name,
					int variable_type,
					void *variable_ptr,
					int debug_flag );

#endif /* __D_PMAC_CS_AXIS_H__ */

