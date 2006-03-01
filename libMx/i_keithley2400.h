/*
 * Name:    i_keithley2400.h
 *
 * Purpose: Header file for MX driver for Keithley 2400 switching multimeters.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2004-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_KEITHLEY2400_H__
#define __I_KEITHLEY2400_H__

typedef struct {
	MX_RECORD *record;

	MX_INTERFACE port_interface;

	long last_source_type;
	long last_measurement_type;
} MX_KEITHLEY2400;

/* 'measurement_type' values for the MX_KEITHLEY2400 structure. */

#define MXT_KEITHLEY2400_INVALID	(-1)
#define MXT_KEITHLEY2400_UNKNOWN	0
#define MXT_KEITHLEY2400_VOLT		1
#define MXT_KEITHLEY2400_CURR		2
#define MXT_KEITHLEY2400_RES		3

MX_API mx_status_type mxi_keithley2400_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxi_keithley2400_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxi_keithley2400_print_structure( FILE *file,
							MX_RECORD *record );
MX_API mx_status_type mxi_keithley2400_open( MX_RECORD *record );
MX_API mx_status_type mxi_keithley2400_close( MX_RECORD *record );


MX_API mx_status_type mxi_keithley2400_get_measurement_type(
						MX_KEITHLEY2400 *keithley2400,
						MX_INTERFACE *port_interface,
						long *measurement_type,
						int debug_flag );

MX_API mx_status_type mxi_keithley2400_set_measurement_type(
						MX_KEITHLEY2400 *keithley2400,
						MX_INTERFACE *port_interface,
						long measurement_type,
						int debug_flag );

MX_API mx_status_type mxi_keithley2400_get_source_type(
						MX_KEITHLEY2400 *keithley2400,
						MX_INTERFACE *port_interface,
						long *source_type,
						int debug_flag );

MX_API mx_status_type mxi_keithley2400_set_source_type(
						MX_KEITHLEY2400 *keithley2400,
						MX_INTERFACE *port_interface,
						long source_type,
						int debug_flag );

extern MX_RECORD_FUNCTION_LIST mxi_keithley2400_record_function_list;

extern long mxi_keithley2400_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_keithley2400_rfield_def_ptr;

#define MXI_KEITHLEY2400_STANDARD_FIELDS \
  {-1, -1, "port_interface", MXFT_INTERFACE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_KEITHLEY2400, port_interface), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __I_KEITHLEY2400_H__ */
