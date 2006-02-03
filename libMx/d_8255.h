/*
 * Name:    d_8255.h
 *
 * Purpose: Header file for MX input and output drivers to control 
 *          Intel 8255 digital input/output registers.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999, 2001-2002, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_8255_H__
#define __D_8255_H__

#include "mx_digital_input.h"
#include "mx_digital_output.h"

/* ===== Intel 8255 input/output register data structures ===== */

typedef struct {
	MX_RECORD *interface_record;
	char port[MX_8255_MAX_PORT_NAME_LENGTH + 1];
	int port_number;
} MX_8255_IN;

typedef struct {
	MX_RECORD *interface_record;
	char port[MX_8255_MAX_PORT_NAME_LENGTH + 1];
	int port_number;
} MX_8255_OUT;

#define MXD_8255_IN_STANDARD_FIELDS \
  {-1, -1, "interface_record", MXFT_RECORD, NULL, 0, {0}, \
        MXF_REC_TYPE_STRUCT, offsetof(MX_8255_IN, interface_record), \
        {0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "port", MXFT_STRING, NULL, 1, {MX_8255_MAX_PORT_NAME_LENGTH}, \
        MXF_REC_TYPE_STRUCT, offsetof(MX_8255_IN, port), \
        {sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#define MXD_8255_OUT_STANDARD_FIELDS \
  {-1, -1, "interface_record", MXFT_RECORD, NULL, 0, {0}, \
        MXF_REC_TYPE_STRUCT, offsetof(MX_8255_OUT, interface_record), \
        {0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "port", MXFT_STRING, NULL, 1, {MX_8255_MAX_PORT_NAME_LENGTH}, \
        MXF_REC_TYPE_STRUCT, offsetof(MX_8255_OUT, port), \
        {sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/* Define all of the interface functions. */

/* First the input functions. */

MX_API mx_status_type mxd_8255_in_initialize_type( long type );
MX_API mx_status_type mxd_8255_in_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_8255_in_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_8255_in_delete_record( MX_RECORD *record );
MX_API mx_status_type mxd_8255_in_print_structure( FILE *file,
							MX_RECORD *record );
MX_API mx_status_type mxd_8255_in_read_parms_from_hardware(
							MX_RECORD *record );
MX_API mx_status_type mxd_8255_in_write_parms_to_hardware(
							MX_RECORD *record );
MX_API mx_status_type mxd_8255_in_open( MX_RECORD *record );
MX_API mx_status_type mxd_8255_in_close( MX_RECORD *record );
MX_API mx_status_type mxd_8255_in_read( MX_DIGITAL_INPUT *dinput );

extern MX_RECORD_FUNCTION_LIST mxd_8255_in_record_function_list;
extern MX_GENERIC_FUNCTION_LIST mxd_8255_in_generic_function_list;
extern MX_DIGITAL_INPUT_FUNCTION_LIST
				mxd_8255_in_digital_input_function_list;

extern mx_length_type mxd_8255_in_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_8255_in_rfield_def_ptr;

/* Second the output functions. */

MX_API mx_status_type mxd_8255_out_initialize_type( long type );
MX_API mx_status_type mxd_8255_out_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_8255_out_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_8255_out_delete_record( MX_RECORD *record );
MX_API mx_status_type mxd_8255_out_print_structure( FILE *file,
							MX_RECORD *record );
MX_API mx_status_type mxd_8255_out_read_parms_from_hardware(
							MX_RECORD *record );
MX_API mx_status_type mxd_8255_out_write_parms_to_hardware(
							MX_RECORD *record );
MX_API mx_status_type mxd_8255_out_open( MX_RECORD *record );
MX_API mx_status_type mxd_8255_out_close( MX_RECORD *record );
MX_API mx_status_type mxd_8255_out_read( MX_DIGITAL_OUTPUT *doutput );
MX_API mx_status_type mxd_8255_out_write( MX_DIGITAL_OUTPUT *doutput );

extern MX_RECORD_FUNCTION_LIST mxd_8255_out_record_function_list;
extern MX_GENERIC_FUNCTION_LIST mxd_8255_out_generic_function_list;
extern MX_DIGITAL_OUTPUT_FUNCTION_LIST
				mxd_8255_out_digital_output_function_list;

extern mx_length_type mxd_8255_out_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_8255_out_rfield_def_ptr;

#endif /* __D_8255_H__ */

