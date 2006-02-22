/*
 * Name:    i_6821.h
 *
 * Purpose: Header for MX driver for Motorola MC6821 digital I/O chips.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2000-2001 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_6821_H__
#define __I_6821_H__

#include "mx_record.h"
#include "mx_generic.h"

/* The available ports are Port A and Port B. */

#define MX_MC6821_PORT_A_DATA		0
#define MX_MC6821_PORT_A_CONTROL	1
#define MX_MC6821_PORT_B_DATA		2
#define MX_MC6821_PORT_B_CONTROL	3

typedef struct {
	MX_RECORD *record;

	MX_RECORD *portio_record;
	unsigned long base_address;

	unsigned long port_a_data_direction;
	unsigned long port_b_data_direction;

	MX_RECORD *port_a_record;
	MX_RECORD *port_b_record;
} MX_6821;

#define MXI_6821_STANDARD_FIELDS \
  {-1, -1, "portio_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_6821, portio_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "base_address", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_6821, base_address), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "port_a_data_direction", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_6821, port_a_data_direction), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "port_b_data_direction", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_6821, port_b_data_direction), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

MX_API mx_status_type mxi_6821_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxi_6821_open( MX_RECORD *record );
MX_API mx_status_type mxi_6821_close( MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxi_6821_record_function_list;
extern MX_GENERIC_FUNCTION_LIST mxi_6821_generic_function_list;

extern long mxi_6821_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_6821_rfield_def_ptr;

#endif /* __I_6821_H__ */
