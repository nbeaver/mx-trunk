/*
 * Name:    i_pcstep.h
 *
 * Purpose: Header for MX interface driver for National Instruments
 *          PC-Step motor controllers (formerly made by nuLogic).
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2000, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_PCSTEP_H__
#define __I_PCSTEP_H__

#include "mx_generic.h"

/* PC-Step commands */

#define MX_PCSTEP_COMMAND_PACKET_TERMINATOR_WORD	0x000A

#define MX_PCSTEP_READ_COMMUNICATIONS_STATUS		0x2041
#define MX_PCSTEP_READ_LIMIT_SWITCH_STATUS		0x2042
#define MX_PCSTEP_SET_STEP_OUTPUT_MODE_AND_POLARITY	0x3043
#define MX_PCSTEP_SET_IO_PORT_POLARITY_AND_DIRECTION	0x3044
#define MX_PCSTEP_SET_IO_PORT_OUTPUT			0x3045
#define MX_PCSTEP_ENABLE_IO_PORT_TRIGGER_INPUTS		0x3046
#define MX_PCSTEP_READ_IO_PORT				0x2047
#define MX_PCSTEP_TRIGGER_IO_PORT_NUMBER		0x3048
#define MX_PCSTEP_READ_PER_AXIS_HW_STATUS(n)		( 0x2049 | ((n) << 8) )
#define MX_PCSTEP_SET_LIMIT_SWITCH_POLARITY		0x304A
#define MX_PCSTEP_ENABLE_LIMIT_SWITCH_INPUTS		0x304B
#define MX_PCSTEP_STOP_MOTION(n)			( 0x204C | ((n) << 8) )
#define MX_PCSTEP_KILL_MOTION(n)			( 0x204D | ((n) << 8) )
#define MX_PCSTEP_LOAD_TARGET_POSITION(n)		( 0x404E | ((n) << 8) )
#define MX_PCSTEP_LOAD_STEPS_PER_SECOND(n)		( 0x404F | ((n) << 8) )
#define MX_PCSTEP_RESET_POSITION_COUNTER_TO_ZERO(n)	( 0x2050 | ((n) << 8) )
#define MX_PCSTEP_FIND_HOME(n)				( 0x3051 | ((n) << 8) )
#define MX_PCSTEP_FIND_ENCODER_INDEX(n)			( 0x4052 | ((n) << 8) )
#define MX_PCSTEP_READ_POSITION(n)			( 0x2053 | ((n) << 8) )
#define MX_PCSTEP_READ_STEPS_PER_SECOND(n)		( 0x2054 | ((n) << 8) )
#define MX_PCSTEP_SET_STOP_MODE(n)			( 0x3055 | ((n) << 8) )
#define MX_PCSTEP_LOAD_FOLLOWING_ERROR(n)		( 0x3056 | ((n) << 8) )
#define MX_PCSTEP_LOAD_STEPS_AND_LINES_PER_REV(n)	( 0x4057 | ((n) << 8) )

#define MX_PCSTEP_READ_ENCODER(n)			( 0x2059 | ((n) << 8) )
#define MX_PCSTEP_SET_LOOP_MODE(n)			( 0x305A | ((n) << 8) )
#define MX_PCSTEP_SET_BASE_VELOCITY(n)			( 0x305B | ((n) << 8) )
#define MX_PCSTEP_MULTI_AXIS_START			0x305C
#define MX_PCSTEP_TRIGGER_BUFFER_DELIMITER		0x205D
#define MX_PCSTEP_INCREMENTAL_VELOCITY_CHANGE(n)	( 0x305E | ((n) << 8) )
#define MX_PCSTEP_BEGIN_TRIGGER_NUMBER_PRESTORE		0x305F
#define MX_PCSTEP_END_TRIGGER_PRESTORE			0x2060
#define MX_PCSTEP_START_MOTION(n)			( 0x2061 | ((n) << 8) )
#define MX_PCSTEP_LOAD_ACCELERATION(n)			( 0x4062 | ((n) << 8) )
#define MX_PCSTEP_LOAD_ACCELERATION_FACTOR(n)		( 0x3063 | ((n) << 8) )
#define MX_PCSTEP_SET_OPERATION_MODE(n)			( 0x3064 | ((n) << 8) )
#define MX_PCSTEP_LOAD_POSITION_BREAKPOINT(n)		( 0x4065 | ((n) << 8) )
#define MX_PCSTEP_ENABLE_BREAKPOINT_FUNCTION(n)		( 0x3066 | ((n) << 8) )

#define MX_PCSTEP_LOAD_POSITION_REFERENCE_OBJECT(n)	( 0x4068 | ((n) << 8) )
#define MX_PCSTEP_LOAD_POSITION_SCALE_FACTOR(n)		( 0x4069 | ((n) << 8) )
#define MX_PCSTEP_SET_SCALE_FACTOR_SEQUENCE(n)		( 0x306A | ((n) << 8) )
#define MX_PCSTEP_SET_RUN_STOP_STATUS_PULSE_WIDTH(n)	( 0x306B | ((n) << 8) )
#define MX_PCSTEP_LOAD_ROTARY_COUNTS(n)			( 0x406C | ((n) << 8) )

#define MX_PCSTEP_SET_BREAKPOINT_TO_TRIGGER_LINK(n)	( 0x306E | ((n) << 8) )
#define MX_PCSTEP_LOAD_BREAKPOINT_REPEAT_PERIOD(n)	( 0x406F | ((n) << 8) )
#define MX_PCSTEP_READ_AD_CONVERTER_ANALOG_INPUT_VALUE	0x3070
#define MX_PCSTEP_READ_AUX_DIGITAL_IO_INPUT_VALUES	0x4071
#define MX_PCSTEP_SET_AUX_DIGITAL_IO_OUTPUT_VALUES	0x4072

/* Communications status byte flags. */

#define MXF_PCSTEP_READY_TO_RECEIVE	0x0100
#define MXF_PCSTEP_RETURN_DATA_PENDING	0x0200

#define MXF_PCSTEP_COMMAND_IN_PROCESS	0x0800
#define MXF_PCSTEP_COMMAND_ERROR	0x1000
#define MXF_PCSTEP_WATCHDOG_RESET	0x2000
#define MXF_PCSTEP_HARDWARE_FAILURE	0x4000
#define MXF_PCSTEP_COMMAND_EXECUTION	0x8000

#define MXF_PCSTEP_ERROR_FLAGS \
		( MXF_PCSTEP_COMMAND_ERROR \
		| MXF_PCSTEP_WATCHDOG_RESET \
		| MXF_PCSTEP_HARDWARE_FAILURE )

/*---------*/

#define MX_PCSTEP_NUM_MOTORS	4

typedef struct {
	MX_RECORD *record;

	MX_RECORD *portio_record;
	unsigned long base_address;

	unsigned long limit_switch_polarity;
	unsigned long enable_limit_switches;

	unsigned long retries;
	unsigned long max_discarded_words;
	unsigned long delay_milliseconds;

	mx_bool_type controller_initialized;
	long home_search_in_progress;

	MX_RECORD *motor_array[MX_PCSTEP_NUM_MOTORS];
} MX_PCSTEP;

#define MXI_PCSTEP_STANDARD_FIELDS \
  {-1, -1, "portio_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCSTEP, portio_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "base_address", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCSTEP, base_address), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "limit_switch_polarity", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCSTEP, limit_switch_polarity), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "enable_limit_switches", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCSTEP, enable_limit_switches), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \

/*---*/

MX_API mx_status_type mxi_pcstep_initialize_type( long type );
MX_API mx_status_type mxi_pcstep_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxi_pcstep_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxi_pcstep_delete_record( MX_RECORD *record );
MX_API mx_status_type mxi_pcstep_print_structure( FILE *file,
							MX_RECORD *record );
MX_API mx_status_type mxi_pcstep_read_parms_from_hardware(
					MX_RECORD *record );
MX_API mx_status_type mxi_pcstep_write_parms_to_hardware(
					MX_RECORD *record );
MX_API mx_status_type mxi_pcstep_open( MX_RECORD *record );
MX_API mx_status_type mxi_pcstep_close( MX_RECORD *record );
MX_API mx_status_type mxi_pcstep_resynchronize( MX_RECORD *record );

/*---*/

MX_API uint16_t mxi_pcstep_get_status_word( MX_PCSTEP *pcstep );

MX_API mx_status_type mxi_pcstep_reset_errors( MX_PCSTEP *pcstep );

MX_API mx_status_type mxi_pcstep_command( MX_PCSTEP *pcstep, int command,
					uint32_t command_argument,
					uint32_t *command_response );

extern MX_RECORD_FUNCTION_LIST mxi_pcstep_record_function_list;
extern MX_GENERIC_FUNCTION_LIST mxi_pcstep_generic_function_list;

extern long mxi_pcstep_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_pcstep_rfield_def_ptr;

#endif /* __I_PCSTEP_H__ */
