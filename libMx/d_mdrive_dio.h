/*
 * Name:    d_mdrive_dio.h
 *
 * Purpose: Header file for MX input and output drivers to control 
 *          the digital I/O ports available on Intelligent Motion
 *          Systems MDrive motor controllers.
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

#ifndef __D_MDRIVE_DIO_H__
#define __D_MDRIVE_DIO_H__

#include "mx_digital_input.h"
#include "mx_digital_output.h"

/* ===== MDrive digital input/output register data structures ===== */

typedef struct {
	MX_RECORD *record;

	MX_RECORD *mdrive_record;
	int32_t port_number;
} MX_MDRIVE_DINPUT;

typedef struct {
	MX_RECORD *record;

	MX_RECORD *mdrive_record;
	int32_t port_number;
} MX_MDRIVE_DOUTPUT;

#define MXD_MDRIVE_DINPUT_STANDARD_FIELDS \
  {-1, -1, "mdrive_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MDRIVE_DINPUT, mdrive_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "port_number", MXFT_INT32, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MDRIVE_DINPUT, port_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#define MXD_MDRIVE_DOUTPUT_STANDARD_FIELDS \
  {-1, -1, "mdrive_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MDRIVE_DOUTPUT, mdrive_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "port_number", MXFT_INT32, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MDRIVE_DOUTPUT, port_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/* Define all of the interface functions. */

/* First the input functions. */

MX_API mx_status_type mxd_mdrive_din_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_mdrive_din_read( MX_DIGITAL_INPUT *dinput );

extern MX_RECORD_FUNCTION_LIST mxd_mdrive_din_record_function_list;
extern MX_DIGITAL_INPUT_FUNCTION_LIST
				mxd_mdrive_din_digital_input_function_list;

extern mx_length_type mxd_mdrive_din_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_mdrive_din_rfield_def_ptr;

/* Second the output functions. */

MX_API mx_status_type mxd_mdrive_dout_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_mdrive_dout_read( MX_DIGITAL_OUTPUT *doutput );
MX_API mx_status_type mxd_mdrive_dout_write( MX_DIGITAL_OUTPUT *doutput );

extern MX_RECORD_FUNCTION_LIST mxd_mdrive_dout_record_function_list;
extern MX_DIGITAL_OUTPUT_FUNCTION_LIST
				mxd_mdrive_dout_digital_output_function_list;

extern mx_length_type mxd_mdrive_dout_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_mdrive_dout_rfield_def_ptr;

#endif /* __D_MDRIVE_DIO_H__ */

