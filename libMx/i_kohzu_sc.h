/*
 * Name:    i_kohzu_sc.h
 *
 * Purpose: Header for MX driver for Kohzu SC-200, SC-400, and SC-800
 *          motor controllers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2004 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_KOHZU_SC_H__
#define __I_KOHZU_SC_H__

#include "mx_record.h"

#include "mx_motor.h"

/* Define the data structures used by a Kohzu SC-x00 interface. */

#define MX_MAX_KOHZU_SC_AXES	8

#define MXU_KOHZU_SC_MAX_COMMAND_LENGTH	200

#define MXT_KOHZU_SC_UNKNOWN	0
#define MXT_KOHZU_SC_200	200
#define MXT_KOHZU_SC_400	400
#define MXT_KOHZU_SC_800	800

typedef struct {
	MX_RECORD *record;

	MX_INTERFACE port_interface;

	long num_axes;
	long controller_type;
	long firmware_version;

	MX_RECORD *motor_array[MX_MAX_KOHZU_SC_AXES];

	char command[MXU_KOHZU_SC_MAX_COMMAND_LENGTH+1];
	char response[MXU_KOHZU_SC_MAX_COMMAND_LENGTH+1];
} MX_KOHZU_SC;


#define MXLV_KOHZU_SC_COMMAND			7001
#define MXLV_KOHZU_SC_RESPONSE			7002
#define MXLV_KOHZU_SC_COMMAND_WITH_RESPONSE	7003

#define MXI_KOHZU_SC_STANDARD_FIELDS \
  {-1, -1, "port_interface", MXFT_INTERFACE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_KOHZU_SC, port_interface), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {MXLV_KOHZU_SC_COMMAND, -1, "command", MXFT_STRING,\
				NULL, 1, {MXU_KOHZU_SC_MAX_COMMAND_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_KOHZU_SC, command), \
	{sizeof(char)}, NULL, 0}, \
  \
  {MXLV_KOHZU_SC_RESPONSE, -1, "response", MXFT_STRING, \
				NULL, 1, {MXU_KOHZU_SC_MAX_COMMAND_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_KOHZU_SC, response), \
	{sizeof(char)}, NULL, 0}, \
  \
  {MXLV_KOHZU_SC_COMMAND_WITH_RESPONSE, -1, \
  			"command_with_response", MXFT_STRING,\
			NULL, 1, {MXU_KOHZU_SC_MAX_COMMAND_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_KOHZU_SC, command), \
	{sizeof(char)}, NULL, 0}

MX_API mx_status_type mxi_kohzu_sc_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxi_kohzu_sc_open( MX_RECORD *record );
MX_API mx_status_type mxi_kohzu_sc_resynchronize( MX_RECORD *record );
MX_API mx_status_type mxi_kohzu_sc_special_processing_setup(
						MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxi_kohzu_sc_record_function_list;

extern long mxi_kohzu_sc_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_kohzu_sc_rfield_def_ptr;

/* === Driver specific functions === */

MX_API mx_status_type mxi_kohzu_sc_command(
		MX_KOHZU_SC *kohzu_sc, char *command,
		char *response, int max_response_length,
		int debug_flag );

MX_API mx_status_type mxi_kohzu_sc_multiaxis_move(
		MX_KOHZU_SC *kohzu_sc, unsigned long num_motors,
		MX_RECORD **motor_record_array, double *motor_position_array,
		int simultaneous_start );

MX_API mx_status_type mxi_kohzu_sc_select_token( char *response_buffer,
					unsigned int token_number,
					char *token_buffer,
					int max_token_length );

#endif /* __I_KOHZU_SC_H__ */

