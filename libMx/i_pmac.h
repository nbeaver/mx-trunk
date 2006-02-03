/*
 * Name:   i_pmac.h
 *
 * Purpose: Header for MX driver for Delta Tau PMAC controllers.  Note that
 *          this is the driver for talking to the PMAC itself.  PMAC-controlled
 *          motors are interfaced via the driver in libmx/d_pmac.c.
 *
 * Author:  William Lavender
 *
 *----------------------------------------------------------------------------
 *
 * Copyright 1999-2003 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_PMAC_H__
#define __I_PMAC_H__

#if HAVE_EPICS
#  include "mx_epics.h"
#endif

#define MX_PMAC_MAX_MOTORS		32

#define MXU_PMAC_VARIABLE_NAME_LENGTH	8

#define MX_PMAC_NUM_STATUS_CHARACTERS	12

#define MX_PMAC_CS_NUM_STATUS_CHARACTERS 18

/* Define the data structures used by a PMAC controller. */

#define MX_PMAC_TYPE_PMAC1		1
#define MX_PMAC_TYPE_PMAC2		2
#define MX_PMAC_TYPE_PMACUL		3

#define MX_PMAC_TYPE_TURBO		1024

#define MX_PMAC_TYPE_TURBO1		( 1 | MX_PMAC_TYPE_TURBO )
#define MX_PMAC_TYPE_TURBO2		( 2 | MX_PMAC_TYPE_TURBO )
#define MX_PMAC_TYPE_TURBOU		( 3 | MX_PMAC_TYPE_TURBO )

/*---*/

#define MX_PMAC_PORT_TYPE_RS232		1
#define MX_PMAC_PORT_TYPE_EPICS_TC	11

#define MX_PMAC_PORT_TYPE_LENGTH	32
#define MX_PMAC_PORT_ARGS_LENGTH	80

#define MX_PMAC_MAX_COMMAND_LENGTH	500

typedef struct {
	MX_RECORD *record;
	int pmac_type;
	int port_type;

	/* PMAC controller PROM version. */

	int major_version;
	int minor_version;

	int i6_variable;  /* affects error reporting */

	/* Parameters shared by all motor axes. */

	MX_RECORD *port_record;

	char port_type_name[MX_PMAC_PORT_TYPE_LENGTH+1];
	char port_args[MX_PMAC_PORT_ARGS_LENGTH+1];
	int num_cards;

	char command[MX_PMAC_MAX_COMMAND_LENGTH+1];
	char response[MX_PMAC_MAX_COMMAND_LENGTH+1];

#if HAVE_EPICS
	MX_EPICS_PV strcmd_pv;
	MX_EPICS_PV strrsp_pv;
#endif

} MX_PMAC;

#define MXLV_PMAC_COMMAND		7001
#define MXLV_PMAC_RESPONSE		7002
#define MXLV_PMAC_COMMAND_WITH_RESPONSE	7003

#define MXI_PMAC_STANDARD_FIELDS \
  {-1, -1, "port_type_name", MXFT_STRING, NULL, 1,{MX_PMAC_PORT_TYPE_LENGTH},\
	MXF_REC_TYPE_STRUCT, offsetof(MX_PMAC, port_type_name), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "port_args", MXFT_STRING, NULL, 1, {MX_PMAC_PORT_ARGS_LENGTH},\
	MXF_REC_TYPE_STRUCT, offsetof(MX_PMAC, port_args), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "num_cards", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PMAC, num_cards), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {MXLV_PMAC_COMMAND, -1, "command", MXFT_STRING,\
				NULL, 1, {MX_PMAC_MAX_COMMAND_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PMAC, command), \
	{sizeof(char)}, NULL, 0}, \
  \
  {MXLV_PMAC_RESPONSE, -1, "response", MXFT_STRING, \
				NULL, 1, {MX_PMAC_MAX_COMMAND_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PMAC, response), \
	{sizeof(char)}, NULL, 0}, \
  \
  {MXLV_PMAC_COMMAND_WITH_RESPONSE, -1, "command_with_response", MXFT_STRING,\
				NULL, 1, {MX_PMAC_MAX_COMMAND_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PMAC, command), \
	{sizeof(char)}, NULL, 0}

#ifndef MX_CR
#define MX_ACK		'\006'
#define MX_BELL		'\007'
#define MX_LF		'\012'
#define MX_CR		'\015'
#define MX_CTRL_Z	'\032'
#endif

MX_API mx_status_type mxi_pmac_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxi_pmac_finish_record_initialization(MX_RECORD *record);
MX_API mx_status_type mxi_pmac_open( MX_RECORD *record );
MX_API mx_status_type mxi_pmac_resynchronize( MX_RECORD *record );
MX_API mx_status_type mxi_pmac_special_processing_setup( MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxi_pmac_record_function_list;

extern mx_length_type mxi_pmac_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_pmac_rfield_def_ptr;

/* === Driver specific functions === */

MX_API mx_status_type mxi_pmac_command( MX_PMAC *pmac, char *command,
		char *response, int response_buffer_length, int debug_flag );

MX_API mx_status_type mxi_pmac_card_command( MX_PMAC *pmac,
		int card_number, char *command,
		char *response, int response_buffer_length, int debug_flag );

MX_API mx_status_type mxi_pmac_get_variable( MX_PMAC *pmac,
		int card_number, char *variable_name, int variable_type,
		void *variable_ptr, int debug_flag );

MX_API mx_status_type mxi_pmac_set_variable( MX_PMAC *pmac,
		int card_number, char *variable_name, int variable_type,
		void *variable_ptr, int debug_flag );

#endif /* __I_PMAC_H__ */

