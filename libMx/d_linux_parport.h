/*
 * Name:    d_linux_parport.h
 *
 * Purpose: Header file for MX input and output drivers to control Linux
 *          parallel ports as digital input/output registers via the
 *          Linux parport driver.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2003 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_LINUX_PARPORT_H__
#define __D_LINUX_PARPORT_H__

#include "mx_digital_input.h"
#include "mx_digital_output.h"

/* ===== LINUX_PARPORT digital I/O data structures ===== */

typedef struct {
	MX_RECORD *interface_record;
	char port_name[MX_LINUX_PARPORT_MAX_PORT_NAME_LENGTH + 1];
	int port_number;
} MX_LINUX_PARPORT_IN;

typedef struct {
	MX_RECORD *interface_record;
	char port_name[MX_LINUX_PARPORT_MAX_PORT_NAME_LENGTH + 1];
	int port_number;
} MX_LINUX_PARPORT_OUT;

#define MXD_LINUX_PARPORT_IN_STANDARD_FIELDS \
  {-1, -1, "interface_record", MXFT_RECORD, NULL, 0, {0}, \
        MXF_REC_TYPE_STRUCT, offsetof(MX_LINUX_PARPORT_IN, interface_record), \
        {0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "port_name", MXFT_STRING, NULL, 1, \
			{MX_LINUX_PARPORT_MAX_PORT_NAME_LENGTH}, \
        MXF_REC_TYPE_STRUCT, offsetof(MX_LINUX_PARPORT_IN, port_name), \
        {sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#define MXD_LINUX_PARPORT_OUT_STANDARD_FIELDS \
  {-1, -1, "interface_record", MXFT_RECORD, NULL, 0, {0}, \
        MXF_REC_TYPE_STRUCT, offsetof(MX_LINUX_PARPORT_OUT, interface_record), \
        {0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "port_name", MXFT_STRING, NULL, 1, \
			{MX_LINUX_PARPORT_MAX_PORT_NAME_LENGTH}, \
        MXF_REC_TYPE_STRUCT, offsetof(MX_LINUX_PARPORT_OUT, port_name), \
        {sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/* Define all of the interface functions. */

/* First the input functions. */

MX_API mx_status_type mxd_linux_parport_in_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_linux_parport_in_finish_record_initialization(
							MX_RECORD *record );

MX_API mx_status_type mxd_linux_parport_in_read( MX_DIGITAL_INPUT *dinput );

extern MX_RECORD_FUNCTION_LIST mxd_linux_parport_in_record_function_list;
extern MX_DIGITAL_INPUT_FUNCTION_LIST
			mxd_linux_parport_in_digital_input_function_list;

extern long mxd_linux_parport_in_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_linux_parport_in_rfield_def_ptr;

/* Second the output functions. */

MX_API mx_status_type mxd_linux_parport_out_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_linux_parport_out_finish_record_initialization(
							MX_RECORD *record );

MX_API mx_status_type mxd_linux_parport_out_read( MX_DIGITAL_OUTPUT *doutput );
MX_API mx_status_type mxd_linux_parport_out_write( MX_DIGITAL_OUTPUT *doutput );

extern MX_RECORD_FUNCTION_LIST mxd_linux_parport_out_record_function_list;
extern MX_DIGITAL_OUTPUT_FUNCTION_LIST
			mxd_linux_parport_out_digital_output_function_list;

extern long mxd_linux_parport_out_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_linux_parport_out_rfield_def_ptr;

#endif /* __D_LINUX_PARPORT_H__ */

