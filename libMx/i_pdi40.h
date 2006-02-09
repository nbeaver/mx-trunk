/*
 * Name:   i_pdi40.h
 *
 * Purpose: Header for MX driver for Prairie Digital, Inc. Model 40
 *          data acquisition and control module.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_PDI40_H__
#define __I_PDI40_H__

#include "mx_record.h"
#include "mx_generic.h"
#include "mx_rs232.h"

#include "mx_motor.h"

/* Define the data structures used by a PDI Model 40 interface. */

typedef struct {
	MX_RECORD *record;

	MX_RECORD *rs232_record;

	/* Parameters shared by all three stepping motor ports. */
	char    stepper_mode;
	int32_t stepper_speed;
	int32_t stepper_stop_delay;

	char    currently_moving_stepper;
	int32_t current_move_distance;

	/* The following is an array of three MX_MOTOR pointers.  There is
	 * one for each of the three stepping motors 'A', 'B', and 'C'.
	 */

	MX_MOTOR *stepper_motor[3];
} MX_PDI40;

#define MXI_PDI40_STANDARD_FIELDS \
  {-1, -1, "rs232_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PDI40, rs232_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "stepper_mode", MXFT_CHAR, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PDI40, stepper_mode), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "stepper_speed", MXFT_INT32, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PDI40, stepper_speed), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "stepper_stop_delay", MXFT_INT32, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PDI40, stepper_stop_delay), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/* The following defines are used with the stepper_motor array above. */

#define MX_PDI40_STEPPER_A	0
#define MX_PDI40_STEPPER_B	1
#define MX_PDI40_STEPPER_C	2

/* Some defines for details of the protocol for talking to the PDI40. */

#define MX_PDI40_END_OF_COMMAND		'\015'
#define MX_PDI40_END_OF_LINE		'\015'
#define MX_PDI40_END_OF_RESPONSE	'>'

MX_API mx_status_type mxi_pdi40_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxi_pdi40_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxi_pdi40_open( MX_RECORD *record );
MX_API mx_status_type mxi_pdi40_close( MX_RECORD *record );

MX_API mx_status_type mxi_pdi40_getline( MX_PDI40 *pdi40,
			char *buffer, int buffer_length, int debug_flag );
MX_API mx_status_type mxi_pdi40_putline( MX_PDI40 *pdi40,
					char *buffer, int debug_flag );
MX_API mx_status_type mxi_pdi40_getc( MX_PDI40 *pdi40,
					char *c, int debug_flag );
MX_API mx_status_type mxi_pdi40_putc( MX_PDI40 *pdi40,
					char c, int debug_flag );

MX_API mx_status_type mxi_pdi40_is_any_motor_busy( MX_PDI40 *pdi40,
					int *a_motor_is_busy );

extern MX_RECORD_FUNCTION_LIST mxi_pdi40_record_function_list;
extern MX_GENERIC_FUNCTION_LIST mxi_pdi40_generic_function_list;

extern mx_length_type mxi_pdi40_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_pdi40_rfield_def_ptr;

#endif /* __I_PDI40_H__ */
