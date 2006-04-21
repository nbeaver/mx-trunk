/*
 * Name:    i_picomotor.h
 *
 * Purpose: Header for MX driver for New Focus picomotor controllers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_PICOMOTOR_H__
#define __I_PICOMOTOR_H__

#include "mx_record.h"

#include "mx_motor.h"

/* Define the data structures used by a Picomotor interface. */

#define MX_MAX_PICOMOTOR_DRIVERS	31

#define MXU_PICOMOTOR_MAX_COMMAND_LENGTH	200
#define MXU_PICOMOTOR_DRIVER_NAME_LENGTH	4

#define MXT_PICOMOTOR_UNKNOWN_CONTROLLER	0
#define MXT_PICOMOTOR_8750_CONTROLLER		8750
#define MXT_PICOMOTOR_8752_CONTROLLER		8752

#define MXT_PICOMOTOR_UNKNOWN_DRIVER		0
#define MXT_PICOMOTOR_8751_DRIVER		8751
#define MXT_PICOMOTOR_8753_DRIVER		8753

typedef struct {
	MX_RECORD *record;

	MX_RECORD *rs232_record;

	int firmware_major_version;
	int firmware_minor_version;
	int firmware_revision;

	long driver_type[MX_MAX_PICOMOTOR_DRIVERS];
	long current_motor_number[MX_MAX_PICOMOTOR_DRIVERS];

	char command[MXU_PICOMOTOR_MAX_COMMAND_LENGTH+1];
	char response[MXU_PICOMOTOR_MAX_COMMAND_LENGTH+1];
} MX_PICOMOTOR_CONTROLLER;

#define MXLV_PICOMOTOR_COMMAND			7001
#define MXLV_PICOMOTOR_RESPONSE			7002
#define MXLV_PICOMOTOR_COMMAND_WITH_RESPONSE	7003

#define MXI_PICOMOTOR_STANDARD_FIELDS \
  {-1, -1, "rs232_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PICOMOTOR_CONTROLLER, rs232_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {MXLV_PICOMOTOR_COMMAND, -1, "command", MXFT_STRING,\
				NULL, 1, {MXU_PICOMOTOR_MAX_COMMAND_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PICOMOTOR_CONTROLLER, command), \
	{sizeof(char)}, NULL, 0}, \
  \
  {MXLV_PICOMOTOR_RESPONSE, -1, "response", MXFT_STRING, \
				NULL, 1, {MXU_PICOMOTOR_MAX_COMMAND_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PICOMOTOR_CONTROLLER, response), \
	{sizeof(char)}, NULL, 0}, \
  \
  {MXLV_PICOMOTOR_COMMAND_WITH_RESPONSE, -1, \
  			"command_with_response", MXFT_STRING,\
			NULL, 1, {MXU_PICOMOTOR_MAX_COMMAND_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PICOMOTOR_CONTROLLER, command), \
	{sizeof(char)}, NULL, 0}

MX_API mx_status_type mxi_picomotor_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxi_picomotor_open( MX_RECORD *record );
MX_API mx_status_type mxi_picomotor_resynchronize( MX_RECORD *record );
MX_API mx_status_type mxi_picomotor_special_processing_setup(
						MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxi_picomotor_record_function_list;

extern long mxi_picomotor_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_picomotor_rfield_def_ptr;

/* === Driver specific functions === */

MX_API mx_status_type mxi_picomotor_command(
		MX_PICOMOTOR_CONTROLLER *picomotor_controller,
		char *command,
		char *response, size_t max_response_length,
		int command_flags );

MX_API mx_status_type mxi_picomotor_getline(
		MX_PICOMOTOR_CONTROLLER *picomotor_controller,
		char *response, size_t max_response_length,
		int command_flags );

/* Values for the command_flags argument to mxi_picomotor_command(). */

#define MXF_PICOMOTOR_DEBUG			0x1
#define MXF_PICOMOTOR_TERMINATE_ON_STATUS_CHAR	0x2

#endif /* __I_PICOMOTOR_H__ */

