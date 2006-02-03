/*
 * Name:    i_pcmotion32.h
 *
 * Purpose: Header for MX interface driver for the National Instruments
 *          PCMOTION32.DLL used with the ValueMotion series of motor
 *          controllers originally sold by nuLogic.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2000-2001, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_PCMOTION32_H__
#define __I_PCMOTION32_H__

#include "mx_generic.h"

/* Communications status byte flags. */

#define MXF_PCMOTION32_READY_TO_RECEIVE		0x01
#define MXF_PCMOTION32_RETURN_DATA_PENDING	0x02

#define MXF_PCMOTION32_COMMAND_IN_PROCESS	0x08
#define MXF_PCMOTION32_COMMAND_ERROR		0x10
#define MXF_PCMOTION32_WATCHDOG_RESET		0x20
#define MXF_PCMOTION32_HARDWARE_FAILURE		0x40
#define MXF_PCMOTION32_COMMAND_EXECUTION	0x80

#define MXF_PCMOTION32_ERROR_FLAGS \
		( MXF_PCMOTION32_COMMAND_ERROR \
		| MXF_PCMOTION32_WATCHDOG_RESET \
		| MXF_PCMOTION32_HARDWARE_FAILURE \
		| MXF_PCMOTION32_COMMAND_EXECUTION )

/*---------*/

#define MX_PCMOTION32_NUM_MOTORS	4

typedef struct {
	MX_RECORD *record;

	int board_id;

	unsigned long limit_switch_polarity;
	unsigned long enable_limit_switches;

	MX_RECORD *motor_array[MX_PCMOTION32_NUM_MOTORS];
} MX_PCMOTION32;

#define MXI_PCMOTION32_STANDARD_FIELDS \
  {-1, -1, "board_id", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCMOTION32, board_id), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "limit_switch_polarity", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCMOTION32, limit_switch_polarity), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "enable_limit_switches", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PCMOTION32, enable_limit_switches), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \

/*---*/

MX_API mx_status_type mxi_pcmotion32_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxi_pcmotion32_print_structure( FILE *file,
							MX_RECORD *record );
MX_API mx_status_type mxi_pcmotion32_open( MX_RECORD *record );

/*---*/

MX_API char *mxi_pcmotion32_strerror( int status_code );

extern MX_RECORD_FUNCTION_LIST mxi_pcmotion32_record_function_list;
extern MX_GENERIC_FUNCTION_LIST mxi_pcmotion32_generic_function_list;

extern mx_length_type mxi_pcmotion32_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_pcmotion32_rfield_def_ptr;

#endif /* __I_PCMOTION32_H__ */
