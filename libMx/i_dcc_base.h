/*
 * Name:    i_dcc_base.h
 *
 * Purpose: Header file for DCC++ and DCC-EX model railroad base stations.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2023 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_DCC_BASE_H__
#define __I_DCC_BASE_H__

/* Values for base_type */

#define MXT_DCC_BASE_AUTO		0
#define MXT_DCC_BASE_PP			1
#define MXT_DCC_BASE_EX			2

/* Values for dcc_base_flags */

#define MXF_DCC_BASE_DEBUG		0x1

/* Values for dcc_base_command_flags */

#define MXF_DCC_BASE_COMMAND_DEBUG	0x1

/*---*/

#define MXU_DCC_BASE_TYPE_NAME_LENGTH	10

#define MXU_DCC_BASE_COMMAND_LENGTH	40
#define MXU_DCC_BASE_RESPONSE_LENGTH	100

typedef struct {
	MX_RECORD *record;

	MX_RECORD *rs232_record;
	unsigned long base_type;
	unsigned long dcc_base_flags;

	char base_type_name[ MXU_DCC_BASE_TYPE_NAME_LENGTH+1 ];

	char command[ MXU_DCC_BASE_COMMAND_LENGTH+1 ];
	char response[ MXU_DCC_BASE_RESPONSE_LENGTH+1 ];
} MX_DCC_BASE;

#define MXI_DCC_BASE_STANDARD_FIELDS \
  {-1, -1, "rs232_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DCC_BASE, rs232_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "base_type", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DCC_BASE, base_type), \
	{0}, NULL, 0 }, \
  \
  {-1, -1, "dcc_base_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DCC_BASE, dcc_base_flags), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "base_type_name", MXFT_STRING, NULL, 1, \
	  				{MXU_DCC_BASE_TYPE_NAME_LENGTH},\
	MXF_REC_TYPE_STRUCT, offsetof(MX_DCC_BASE, base_type_name), \
	{sizeof(char)}, NULL, 0 }, \
  \
  {-1, -1, "command", MXFT_STRING, NULL, 1, {MXU_DCC_BASE_COMMAND_LENGTH},\
	MXF_REC_TYPE_STRUCT, offsetof(MX_DCC_BASE, command), \
	{sizeof(char)}, NULL, 0 }, \
  \
  {-1, -1, "response", MXFT_STRING, NULL, 1, {MXU_DCC_BASE_RESPONSE_LENGTH},\
	MXF_REC_TYPE_STRUCT, offsetof(MX_DCC_BASE, response), \
	{sizeof(char)}, NULL, 0 }

MX_API mx_status_type mxi_dcc_base_create_record_structures( MX_RECORD *record);

MX_API mx_status_type mxi_dcc_base_open( MX_RECORD *record );

MX_API mx_status_type mxi_dcc_base_special_processing_setup( MX_RECORD *record);

extern MX_RECORD_FUNCTION_LIST mxi_dcc_base_record_function_list;

extern long mxi_dcc_base_num_record_fields;

extern MX_RECORD_FIELD_DEFAULTS *mxi_dcc_base_rfield_def_ptr;

MX_API mx_status_type
mxi_dcc_base_command( MX_DCC_BASE *dcc_base,
		unsigned long dcc_base_command_flags );

#endif /* __I_DCC_BASE_H__ */

