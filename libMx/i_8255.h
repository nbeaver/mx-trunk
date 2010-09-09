/*
 * Name:    i_8255.h
 *
 * Purpose: Header for MX driver for Intel 8255 digital I/O chips and
 *          compatibles.  This driver only assumes the presence of 
 *          8255 mode 0.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2006, 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_8255_H__
#define __I_8255_H__

#include "mx_record.h"
#include "mx_generic.h"

/* The available ports are Port A, Port B, Port CL, Port CH, and Port D.
 */

#define MX_8255_NUM_PORTS	4

#define MX_8255_MAX_RECORDS	5

#define MX_8255_PORT_A		0
#define MX_8255_PORT_B		1
#define MX_8255_PORT_C		2
#define MX_8255_PORT_D		3	/* configuration port */

#define MX_8255_PORT_CL		21
#define MX_8255_PORT_CH		22

#define MXF_8255_ACTIVE		0x80
#define MXF_8255_PORT_A_INPUT	0x10
#define MXF_8255_PORT_B_INPUT	0x02
#define MXF_8255_PORT_CL_INPUT	0x01
#define MXF_8255_PORT_CH_INPUT	0x08

#define MX_8255_PORT_A_NAME	"A"
#define MX_8255_PORT_B_NAME	"B"
#define MX_8255_PORT_C_NAME	"C"
#define MX_8255_PORT_CL_NAME	"CL"
#define MX_8255_PORT_CH_NAME	"CH"
#define MX_8255_PORT_D_NAME	"D"

#define MX_8255_MAX_PORT_NAME_LENGTH	2

typedef struct {
	MX_RECORD *record;

	MX_RECORD *portio_record;
	unsigned long base_address;

	uint8_t port_value[MX_8255_NUM_PORTS];
	MX_RECORD *port_array[MX_8255_MAX_RECORDS];

	int delayed_initialization_in_progress;
} MX_8255;

#define MXI_8255_STANDARD_FIELDS \
  {-1, -1, "portio_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_8255, portio_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "base_address", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_8255, base_address), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

MX_API mx_status_type mxi_8255_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxi_8255_open( MX_RECORD *record );
MX_API mx_status_type mxi_8255_close( MX_RECORD *record );
MX_API mx_status_type mxi_8255_finish_delayed_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxi_8255_resynchronize( MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxi_8255_record_function_list;
extern MX_GENERIC_FUNCTION_LIST mxi_8255_generic_function_list;

extern long mxi_8255_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_8255_rfield_def_ptr;

/* === Driver specific functions === */

MX_API mx_status_type mxi_8255_read_port( MX_8255 *i8255,
				int port_number, uint8_t *value );

MX_API mx_status_type mxi_8255_write_port( MX_8255 *i8255,
				int port_number, uint8_t value );

MX_API void mxi_8255_update_outputs( MX_8255 *i8255 );

#endif /* __I_8255_H__ */
