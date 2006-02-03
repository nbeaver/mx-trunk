/*
 * Name:    i_keithley2700.h
 *
 * Purpose: Header file for MX driver for Keithley 2700 switching multimeters.
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

#ifndef __I_KEITHLEY2700_H__
#define __I_KEITHLEY2700_H__

typedef struct {
	MX_RECORD *record;

	MX_INTERFACE port_interface;

	int num_slots;

	int *module_type;
	int *num_channels;
	int **channel_type;
} MX_KEITHLEY2700;

/* 'channel_type' values for the MX_KEITHLEY2700 structure. */

#define MXT_KEITHLEY2700_INVALID	(-1)
#define MXT_KEITHLEY2700_UNKNOWN	0
#define MXT_KEITHLEY2700_DCV		1
#define MXT_KEITHLEY2700_ACV		2
#define MXT_KEITHLEY2700_DCI		3
#define MXT_KEITHLEY2700_ACI		4
#define MXT_KEITHLEY2700_OHMS_2		5
#define MXT_KEITHLEY2700_OHMS_4		6
#define MXT_KEITHLEY2700_FREQ		7
#define MXT_KEITHLEY2700_PERIOD		8
#define MXT_KEITHLEY2700_TEMP		9
#define MXT_KEITHLEY2700_CONT		10

/* Macros for splitting the system channel number into slot and channel. */

#define mxi_keithley2700_slot(x)	((x)/100)
#define mxi_keithley2700_channel(x)	((x)%100)

MX_API mx_status_type mxi_keithley2700_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxi_keithley2700_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxi_keithley2700_print_structure( FILE *file,
							MX_RECORD *record );
MX_API mx_status_type mxi_keithley2700_open( MX_RECORD *record );
MX_API mx_status_type mxi_keithley2700_close( MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxi_keithley2700_record_function_list;

extern mx_length_type mxi_keithley2700_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_keithley2700_rfield_def_ptr;

#define MXI_KEITHLEY2700_STANDARD_FIELDS \
  {-1, -1, "port_interface", MXFT_INTERFACE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_KEITHLEY2700, port_interface), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __I_KEITHLEY2700_H__ */
