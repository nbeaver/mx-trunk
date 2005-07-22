/*
 * Name:    i_lpt.h
 *
 * Purpose: Header for MX driver for using a PC compatible LPT printer port
 *          for digital I/O.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2002-2003 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_LPT_H__
#define __I_LPT_H__

#define MX_LPT_DATA_PORT		0
#define MX_LPT_STATUS_PORT		1
#define MX_LPT_CONTROL_PORT		2

#define MX_LPT_DATA_PORT_NAME		"data"
#define MX_LPT_STATUS_PORT_NAME		"status"
#define MX_LPT_CONTROL_PORT_NAME	"control"

#define MX_LPT_MAX_PORT_NAME_LENGTH	7

#define MX_LPT_SPP_MODE			0
#define MX_LPT_EPP_MODE			1
#define MX_LPT_ECP_MODE			2

#define MX_LPT_SPP_MODE_NAME		"SPP"
#define MX_LPT_EPP_MODE_NAME		"EPP"
#define MX_LPT_ECP_MODE_NAME		"ECP"

#define MXU_LPT_MODE_NAME_LENGTH	3

/* 'lpt_flags' values. */

#define MXF_LPT_INVERT_INVERTED_BITS	0x1

typedef struct {
	MX_RECORD *record;

	MX_RECORD *portio_record;
	unsigned long base_address;

	int lpt_mode;
	char lpt_mode_name[ MXU_LPT_MODE_NAME_LENGTH + 1 ];

	unsigned long lpt_flags;

	mx_uint8_type data_port_value;
	mx_uint8_type status_port_value;
	mx_uint8_type control_port_value;
} MX_LPT;

#define MXI_LPT_STANDARD_FIELDS \
  {-1, -1, "portio_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_LPT, portio_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "base_address", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_LPT, base_address), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "lpt_mode_name", MXFT_STRING, NULL, 1, {MXU_LPT_MODE_NAME_LENGTH+1},\
	MXF_REC_TYPE_STRUCT, offsetof(MX_LPT, lpt_mode_name), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "lpt_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_LPT, lpt_flags), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION}

MX_API mx_status_type mxi_lpt_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxi_lpt_finish_record_initialization( MX_RECORD *record );
MX_API mx_status_type mxi_lpt_open( MX_RECORD *record );
MX_API mx_status_type mxi_lpt_close( MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxi_lpt_record_function_list;

extern long mxi_lpt_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_lpt_rfield_def_ptr;

/* === Driver specific functions === */

MX_API mx_status_type mxi_lpt_read_port( MX_LPT *lpt,
				int port_number, mx_uint8_type *value );

MX_API mx_status_type mxi_lpt_write_port( MX_LPT *lpt,
				int port_number, mx_uint8_type value );

MX_API void mxi_lpt_update_outputs( MX_LPT *lpt );

#endif /* __I_LPT_H__ */
