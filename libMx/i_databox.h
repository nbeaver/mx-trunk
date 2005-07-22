/*
 * Name:    i_databox.h
 *
 * Purpose: Header for MX driver for the Radix Instruments Databox.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2000-2001 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_DATABOX_H__
#define __I_DATABOX_H__

#include "mx_record.h"
#include "mx_generic.h"

#include "mx_motor.h"
#include "mx_rs232.h"

#define MX_MAX_DATABOX_AXES	3

#define MX_DATABOX_AXIS_NAMES	"XYZ"

/* Databox command modes. */

#define MX_DATABOX_UNKNOWN_MODE		0
#define MX_DATABOX_MONITOR_MODE		1
#define MX_DATABOX_CALIBRATE_MODE	2
#define MX_DATABOX_EDIT_MODE		3
#define MX_DATABOX_RUN_MODE		4
#define MX_DATABOX_OPTION_MODE		5

/* Databox limit modes. */

#define MX_DATABOX_UNKNOWN_LIMIT_MODE	0
#define MX_DATABOX_CONSTANT_TIME_MODE	1
#define MX_DATABOX_CONSTANT_COUNTS_MODE	2

/* Start action types */

#define MX_DATABOX_NO_ACTION		0
#define MX_DATABOX_MCS_START_ACTION	1
#define MX_DATABOX_COUNTER_START_ACTION	2
#define MX_DATABOX_FREE_MOVE_ACTION	3	/* move via calibrate mode */
#define MX_DATABOX_MCS_MOVE_ACTION	4	/* move via a sequence */

typedef struct {
	MX_RECORD *record;

	MX_RECORD *rs232_record;
	double maximum_stepping_rate;
	int use_external_time_base;
	int command_mode;
	int limit_mode;
	char moving_motor;
	int last_start_action;
	double degrees_per_x_step;

	MX_RECORD *motor_record_array[MX_MAX_DATABOX_AXES];
	MX_RECORD *mcs_record;
} MX_DATABOX;

#define MXI_DATABOX_STANDARD_FIELDS \
  {-1, -1, "rs232_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DATABOX, rs232_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "use_external_time_base", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DATABOX, use_external_time_base), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "maximum_stepping_rate", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DATABOX, maximum_stepping_rate), \
	{0}, NULL, MXFF_IN_DESCRIPTION}

#ifndef MX_CR
#define MX_CR	'\015'
#define MX_LF	'\012'
#endif

MX_API mx_status_type mxi_databox_initialize_type( long type );
MX_API mx_status_type mxi_databox_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxi_databox_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxi_databox_delete_record( MX_RECORD *record );
MX_API mx_status_type mxi_databox_read_parms_from_hardware(
					MX_RECORD *record );
MX_API mx_status_type mxi_databox_write_parms_to_hardware(
					MX_RECORD *record );
MX_API mx_status_type mxi_databox_open( MX_RECORD *record );
MX_API mx_status_type mxi_databox_close( MX_RECORD *record );
MX_API mx_status_type mxi_databox_resynchronize( MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxi_databox_record_function_list;

extern long mxi_databox_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_databox_rfield_def_ptr;

/* === Driver specific functions === */

MX_API mx_status_type mxi_databox_command(
	MX_DATABOX *databox,
	char *command, char *response, int response_buffer_length,
	int debug_flag );

MX_API mx_status_type mxi_databox_putline(
	MX_DATABOX *databox,
	char *command,
	int debug_flag );

MX_API mx_status_type mxi_databox_getline(
	MX_DATABOX *databox,
	char *response, int response_buffer_length,
	int debug_flag );

MX_API mx_status_type mxi_databox_putchar(
	MX_DATABOX *databox,
	char c,
	int debug_flag );

MX_API mx_status_type mxi_databox_getchar(
	MX_DATABOX *databox,
	char *c,
	int debug_flag );

MX_API mx_status_type mxi_databox_get_command_mode( MX_DATABOX *databox );

MX_API mx_status_type mxi_databox_set_command_mode( MX_DATABOX *databox,
							int mode,
							int discard_response );

MX_API mx_status_type mxi_databox_get_limit_mode( MX_DATABOX *databox );

MX_API mx_status_type mxi_databox_set_limit_mode( MX_DATABOX *databox,
							int mode );

MX_API mx_status_type mxi_databox_get_record_from_motor_name(
						MX_DATABOX *databox,
						char motor_name,
						MX_RECORD **motor_record );

MX_API mx_status_type mxi_databox_define_sequence( MX_DATABOX *databox,
						double initial_angle,
						double final_angle,
						double step_size,
						long count_value );

MX_API mx_status_type mxi_databox_discard_unread_input(
						MX_DATABOX *databox,
						int debug_flag );

#endif /* __I_DATABOX_H__ */
