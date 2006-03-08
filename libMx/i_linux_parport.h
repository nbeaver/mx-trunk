/*
 * Name:    i_linux_parport.h
 *
 * Purpose: Header for MX interface driver to control Linux parallel ports
 *          as digital input/output registers via the Linux parport driver.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2003, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_LINUX_PARPORT_H__
#define __I_LINUX_PARPORT_H__

#define MX_LINUX_PARPORT_DATA_PORT		0
#define MX_LINUX_PARPORT_STATUS_PORT		1
#define MX_LINUX_PARPORT_CONTROL_PORT		2

#define MX_LINUX_PARPORT_DATA_PORT_NAME		"data"
#define MX_LINUX_PARPORT_STATUS_PORT_NAME	"status"
#define MX_LINUX_PARPORT_CONTROL_PORT_NAME	"control"

#define MX_LINUX_PARPORT_MAX_PORT_NAME_LENGTH	8

#define MXU_LINUX_PARPORT_NAME_LENGTH		40
#define MXU_LINUX_PARPORT_MODE_NAME_LENGTH	6

/* 'linux_parport_flags' values. */

#define MXF_LINUX_PARPORT_INVERT_INVERTED_BITS		0x1
#define MXF_LINUX_PARPORT_DATA_PORT_REVERSE_MODE	0x2

typedef struct {
	MX_RECORD *record;

	char parport_name[ MXU_LINUX_PARPORT_NAME_LENGTH + 1 ];
	int parport_fd;

	int parport_mode;
	char parport_mode_name[ MXU_LINUX_PARPORT_MODE_NAME_LENGTH + 1 ];

	unsigned long parport_flags;

	uint8_t data_port_value;
	uint8_t status_port_value;
	uint8_t control_port_value;
} MX_LINUX_PARPORT;

#define MXI_LINUX_PARPORT_STANDARD_FIELDS \
  {-1, -1, "parport_name", MXFT_STRING, NULL, 1, \
	  		{MXU_LINUX_PARPORT_NAME_LENGTH + 1}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_LINUX_PARPORT, parport_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "parport_mode_name", MXFT_STRING, NULL, 1, \
			{MXU_LINUX_PARPORT_MODE_NAME_LENGTH + 1},\
	MXF_REC_TYPE_STRUCT, offsetof(MX_LINUX_PARPORT, parport_mode_name), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "parport_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_LINUX_PARPORT, parport_flags), \
	{0}, NULL, MXFF_IN_DESCRIPTION}

MX_API mx_status_type mxi_linux_parport_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxi_linux_parport_open( MX_RECORD *record );
MX_API mx_status_type mxi_linux_parport_close( MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxi_linux_parport_record_function_list;

extern long mxi_linux_parport_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_linux_parport_rfield_def_ptr;

/* === Driver specific functions === */

MX_API mx_status_type mxi_linux_parport_read_port(
				MX_LINUX_PARPORT *linux_parport,
				int port_number,
				uint8_t *value );

MX_API mx_status_type mxi_linux_parport_write_port(
				MX_LINUX_PARPORT *linux_parport,
				int port_number,
				uint8_t value );

MX_API void mxi_linux_parport_update_outputs( MX_LINUX_PARPORT *linux_parport );

#endif /* __I_LINUX_PARPORT_H__ */
