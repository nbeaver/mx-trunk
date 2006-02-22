/*
 * Name:    i_keithley2000.h
 *
 * Purpose: Header file for MX driver for Keithley 2000 series multimeters.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_KEITHLEY2000_H__
#define __I_KEITHLEY2000_H__

typedef struct {
	MX_RECORD *record;

	MX_INTERFACE port_interface;

	int last_measurement_type;
} MX_KEITHLEY2000;

/* 'measurement_type' values. */

#define MXT_KEITHLEY2000_INVALID	(-1)
#define MXT_KEITHLEY2000_UNKNOWN	0
#define MXT_KEITHLEY2000_DCV		1
#define MXT_KEITHLEY2000_ACV		2
#define MXT_KEITHLEY2000_DCI		3
#define MXT_KEITHLEY2000_ACI		4
#define MXT_KEITHLEY2000_OHMS_2		5
#define MXT_KEITHLEY2000_OHMS_4		6
#define MXT_KEITHLEY2000_FREQ		7
#define MXT_KEITHLEY2000_PERIOD		8
#define MXT_KEITHLEY2000_TEMP		9
#define MXT_KEITHLEY2000_CONT		10
#define MXT_KEITHLEY2000_DIODE		11

MX_API mx_status_type mxi_keithley2000_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxi_keithley2000_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxi_keithley2000_print_structure( FILE *file,
							MX_RECORD *record );
MX_API mx_status_type mxi_keithley2000_open( MX_RECORD *record );
MX_API mx_status_type mxi_keithley2000_close( MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxi_keithley2000_record_function_list;

extern long mxi_keithley2000_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_keithley2000_rfield_def_ptr;

#define MXI_KEITHLEY2000_STANDARD_FIELDS \
  {-1, -1, "port_interface", MXFT_INTERFACE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_KEITHLEY2000, port_interface), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __I_KEITHLEY2000_H__ */
