/*
 * Name:   i_powerpmac.h
 *
 * Purpose: Header for MX driver for Delta Tau POWERPMAC controllers.
 *
 * Author:  William Lavender
 *
 *----------------------------------------------------------------------------
 *
 * Copyright 1999-2003, 2006, 2009-2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_POWERPMAC_H__
#define __I_POWERPMAC_H__

#include "mx_socket.h"

#if HAVE_EPICS
#  include "mx_epics.h"
#endif

#define MX_POWERPMAC_MAX_MOTORS			32

#define MXU_POWERPMAC_VARIABLE_NAME_LENGTH		8

#define MX_POWERPMAC_NUM_STATUS_CHARACTERS		12

#define MX_POWERPMAC_CS_NUM_STATUS_CHARACTERS	18

#define MX_POWER_POWERPMAC_NUM_STATUS_CHARACTERS	16

/* Define the data structures used by a POWERPMAC controller. */

#define MX_POWERPMAC_TYPE_PMAC1		1
#define MX_POWERPMAC_TYPE_PMAC2		2
#define MX_POWERPMAC_TYPE_PMACUL		3

#define MX_POWERPMAC_TYPE_TURBO		1024

#define MX_POWERPMAC_TYPE_TURBO1		( 1 | MX_PMAC_TYPE_TURBO )
#define MX_POWERPMAC_TYPE_TURBO2		( 2 | MX_PMAC_TYPE_TURBO )
#define MX_POWERPMAC_TYPE_TURBOU		( 3 | MX_PMAC_TYPE_TURBO )

#define MX_POWERPMAC_TYPE_POWERPMAC		2048

/*---*/

#define MX_POWERPMAC_PORT_TYPE_RS232		1
#define MX_POWERPMAC_PORT_TYPE_TCP		2
#define MX_POWERPMAC_PORT_TYPE_UDP		3

#define MX_POWERPMAC_PORT_TYPE_GPASCII	11
#define MX_POWERPMAC_PORT_TYPE_GPLIB		12

#define MX_POWERPMAC_PORT_TYPE_EPICS_TC	101

#define MX_POWERPMAC_PORT_TYPE_LENGTH	32
#define MX_POWERPMAC_PORT_ARGS_LENGTH	80

/*---*/

#define MX_POWERPMAC_TCP_PORT_NUMBER		1025
#define MX_POWERPMAC_UDP_PORT_NUMBER		1025

/*---*/

#define MX_POWERPMAC_MAX_COMMAND_LENGTH	500

typedef struct {
	MX_RECORD *record;
	long powerpmac_type;
	long port_type;

	/* POWERPMAC controller PROM version. */

	long major_version;
	long minor_version;

	long error_reporting_mode;

	/* Parameters shared by all motor axes. */

	MX_RECORD *port_record;

	char port_type_name[MX_POWERPMAC_PORT_TYPE_LENGTH+1];
	char port_args[MX_POWERPMAC_PORT_ARGS_LENGTH+1];
	long num_cards;

	char command[MX_POWERPMAC_MAX_COMMAND_LENGTH+1];
	char response[MX_POWERPMAC_MAX_COMMAND_LENGTH+1];

	/* Parameters used by the POWERPMAC 'ethernet' port type. */

	MX_SOCKET *powerpmac_socket;
	char hostname[MXU_HOSTNAME_LENGTH+1];

	/* Parameters used by the PowerPOWERPMAC 'gpascii' port type. */

	char gpascii_username[80];
	char gpascii_password[80];

	/* Parameters used by the PowerPOWERPMAC 'gplib' port type. */

	mx_bool_type gplib_initialized;

#if HAVE_EPICS
	/* Parameters used by the POWERPMAC 'ethernet' port type. */

	MX_EPICS_PV strcmd_pv;
	MX_EPICS_PV strrsp_pv;
#endif

} MX_POWERPMAC;

#define MXLV_POWERPMAC_COMMAND		7001
#define MXLV_POWERPMAC_RESPONSE		7002
#define MXLV_POWERPMAC_COMMAND_WITH_RESPONSE	7003

#define MXI_POWERPMAC_STANDARD_FIELDS \
  {-1, -1, "port_type_name", MXFT_STRING, NULL, 1,{MX_POWERPMAC_PORT_TYPE_LENGTH},\
	MXF_REC_TYPE_STRUCT, offsetof(MX_POWERPMAC, port_type_name), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "port_args", MXFT_STRING, NULL, 1, {MX_POWERPMAC_PORT_ARGS_LENGTH},\
	MXF_REC_TYPE_STRUCT, offsetof(MX_POWERPMAC, port_args), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "num_cards", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_POWERPMAC, num_cards), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
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
  {MXLV_POWERPMAC_COMMAND_WITH_RESPONSE, -1, "command_with_response", MXFT_STRING,\
				NULL, 1, {MX_POWERPMAC_MAX_COMMAND_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_POWERPMAC, command), \
	{sizeof(char)}, NULL, 0}

#ifndef MX_CR
#define MX_ACK		'\006'
#define MX_BELL		'\007'
#define MX_LF		'\012'
#define MX_CR		'\015'
#define MX_CTRL_Z	'\032'
#endif

MX_API mx_status_type mxi_powerpmac_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxi_powerpmac_finish_record_initialization(MX_RECORD *record);
MX_API mx_status_type mxi_powerpmac_open( MX_RECORD *record );
MX_API mx_status_type mxi_powerpmac_resynchronize( MX_RECORD *record );
MX_API mx_status_type mxi_powerpmac_special_processing_setup( MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxi_powerpmac_record_function_list;

extern long mxi_powerpmac_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_powerpmac_rfield_def_ptr;

/* === Driver specific functions === */

MX_API mx_status_type mxi_powerpmac_command( MX_POWERPMAC *powerpmac, char *command,
		char *response, size_t response_buffer_length,
		mx_bool_type debug_flag );

MX_API mx_status_type mxi_powerpmac_card_command( MX_POWERPMAC *powerpmac,
		long card_number, char *command,
		char *response, size_t response_buffer_length,
		mx_bool_type debug_flag );

MX_API mx_status_type mxi_powerpmac_get_variable( MX_POWERPMAC *powerpmac,
		long card_number, char *variable_name, long variable_type,
		void *variable_ptr, mx_bool_type debug_flag );

MX_API mx_status_type mxi_powerpmac_set_variable( MX_POWERPMAC *powerpmac,
		long card_number, char *variable_name, long variable_type,
		void *variable_ptr, mx_bool_type debug_flag );

#endif /* __I_POWERPMAC_H__ */

