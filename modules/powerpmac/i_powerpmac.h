/*
 * Name:   i_powerpmac.h
 *
 * Purpose: Header for MX driver for Delta Tau PowerPMAC controllers.
 *
 * Author:  William Lavender
 *
 *----------------------------------------------------------------------------
 *
 * Copyright 2010, 2013 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_POWERPMAC_H__
#define __I_POWERPMAC_H__

/*---*/

/* Flag bits for 'powerpmac_flags'. */

#define MXF_POWERPMAC_SHOW_CONFIG		0x1

/*---*/

#define MXU_POWERPMAC_VARIABLE_NAME_LENGTH	80

#define MX_POWERPMAC_MAX_COMMAND_LENGTH		500

#define MX_POWERPMAC_NUM_STATUS_CHARACTERS	16

typedef struct {
	MX_RECORD *record;

	unsigned long powerpmac_flags;

	long major_version;
	long minor_version;

	char command[MX_POWERPMAC_MAX_COMMAND_LENGTH+1];
	char response[MX_POWERPMAC_MAX_COMMAND_LENGTH+1];

	struct SHM *shared_mem;
} MX_POWERPMAC;

#define MXLV_POWERPMAC_COMMAND			7001
#define MXLV_POWERPMAC_RESPONSE			7002
#define MXLV_POWERPMAC_COMMAND_WITH_RESPONSE	7003

#define MXI_POWERPMAC_STANDARD_FIELDS \
  {-1, -1, "powerpmac_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_POWERPMAC, powerpmac_flags), \
	{0}, NULL,  MXFF_IN_DESCRIPTION}, \
  \
  {MXLV_POWERPMAC_COMMAND, -1, "command", MXFT_STRING,\
				NULL, 1, {MX_POWERPMAC_MAX_COMMAND_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_POWERPMAC, command), \
	{sizeof(char)}, NULL, 0}, \
  \
  {MXLV_POWERPMAC_RESPONSE, -1, "response", MXFT_STRING, \
				NULL, 1, {MX_POWERPMAC_MAX_COMMAND_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_POWERPMAC, response), \
	{sizeof(char)}, NULL, 0}, \
  \
  {MXLV_POWERPMAC_COMMAND_WITH_RESPONSE, -1, \
				"command_with_response", MXFT_STRING,\
				NULL, 1, {MX_POWERPMAC_MAX_COMMAND_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_POWERPMAC, command), \
	{sizeof(char)}, NULL, 0}

MX_API mx_status_type mxi_powerpmac_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxi_powerpmac_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxi_powerpmac_open( MX_RECORD *record );
MX_API mx_status_type mxi_powerpmac_special_processing_setup(
							MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxi_powerpmac_record_function_list;

extern long mxi_powerpmac_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_powerpmac_rfield_def_ptr;

/* === Driver specific functions === */

MX_API mx_status_type mxi_powerpmac_command( MX_POWERPMAC *powerpmac,
				char *command,
				char *response, size_t response_buffer_length,
				mx_bool_type debug_flag );

MX_API long mxi_powerpmac_get_long( MX_POWERPMAC *powerpmac,
					char *command,
					mx_bool_type debug_flag );

MX_API double mxi_powerpmac_get_double( MX_POWERPMAC *powerpmac,
					char *command,
					mx_bool_type debug_flag );

#endif /* __I_POWERPMAC_H__ */

