/*
 * Name:    i_vme58.h
 *
 * Purpose: Header for MX driver for Oregon Microsystems VME58 motor
 *          controllers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2000-2002, 2006, 2012 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_VME58_H__
#define __I_VME58_H__

#define MXI_VME58_COMMAND_DELAY		TRUE

#include "mx_record.h"

#include "mx_motor.h"
#include "mx_rs232.h"

/* Define the data structures used by a VME58 controller. */

#define MX_MAX_VME58_AXES		8

#define MXT_VME58_UNKNOWN		0
#define MXT_VME58_8			1
#define MXT_VME58_4E			2

#define MX_VME58_AXIS_NAMES		"XYZTUVRS"

#define MX_VME58_BUFFER_SIZE		4096

/* Define some important addresses in the VME58's address space. */

#define MX_VME58_INPUT_PUT_INDEX	0x000
#define MX_VME58_OUTPUT_GET_INDEX	0x002

#define MX_VME58_OUTPUT_PUT_INDEX	0x800
#define MX_VME58_INPUT_GET_INDEX	0x802

#define MX_VME58_INPUT_BUFFER		0x004
#define MX_VME58_INPUT_BUFFER_END	0x203

#define MX_VME58_OUTPUT_BUFFER		0x804
#define MX_VME58_OUTPUT_BUFFER_END	0xA03

#define MX_VME58_CONTROL_REGISTER	0xFE1
#define MX_VME58_STATUS_REGISTER	0xFE3

typedef struct {
	MX_RECORD *record;

	MX_RECORD *vme_record;
	unsigned long crate_number;
	unsigned long base_address;

	long controller_type;
	long num_axes;

	MX_RECORD *motor_array[MX_MAX_VME58_AXES];

#if MXI_VME58_COMMAND_DELAY
	MX_CLOCK_TICK event_interval;
	MX_CLOCK_TICK next_command_must_be_after_this_tick;
#endif

} MX_VME58;

#define MXI_VME58_STANDARD_FIELDS \
  {-1, -1, "vme_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_VME58, vme_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "crate_number", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_VME58, crate_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "base_address", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_VME58, base_address), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "controller_type", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_VME58, controller_type), \
	{0}, NULL, 0}, \
  \
  {-1, -1, "num_axes", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_VME58, num_axes), \
	{0}, NULL, 0}


MX_API mx_status_type mxi_vme58_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxi_vme58_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxi_vme58_open( MX_RECORD *record );
MX_API mx_status_type mxi_vme58_close( MX_RECORD *record );
MX_API mx_status_type mxi_vme58_resynchronize( MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxi_vme58_record_function_list;

extern long mxi_vme58_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_vme58_rfield_def_ptr;

/* === Driver specific functions === */

MX_API mx_status_type mxi_vme58_command(
	MX_VME58 *vme58, char *command,
	char *response, int response_buffer_length,
	int debug_flag );

#endif /* __I_VME58_H__ */
