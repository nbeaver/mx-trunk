/*
 * Name:    d_picomotor_dio.h
 *
 * Purpose: Header file for MX input and output drivers to control 
 *          Picomotor digital I/O ports.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2004 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_PICOMOTOR_DIO_H__
#define __D_PICOMOTOR_DIO_H__

#include "mx_digital_input.h"
#include "mx_digital_output.h"

/* ===== Picomotor digital input/output register data structures ===== */

typedef struct {
	MX_RECORD *record;

	MX_RECORD *picomotor_controller_record;
	char driver_name[MXU_PICOMOTOR_DRIVER_NAME_LENGTH+1];
	int channel_number;
} MX_PICOMOTOR_DINPUT;

typedef struct {
	MX_RECORD *record;

	MX_RECORD *picomotor_controller_record;
	char driver_name[MXU_PICOMOTOR_DRIVER_NAME_LENGTH+1];
	int channel_number;
} MX_PICOMOTOR_DOUTPUT;

#define MXD_PICOMOTOR_DINPUT_STANDARD_FIELDS \
  {-1, -1, "picomotor_controller_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_PICOMOTOR_DINPUT, picomotor_controller_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "driver_name", MXFT_STRING, NULL, \
	  			1, {MXU_PICOMOTOR_DRIVER_NAME_LENGTH},\
	MXF_REC_TYPE_STRUCT, offsetof(MX_PICOMOTOR_DINPUT, driver_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "channel_number", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PICOMOTOR_DINPUT, channel_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#define MXD_PICOMOTOR_DOUTPUT_STANDARD_FIELDS \
  {-1, -1, "picomotor_controller_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_PICOMOTOR_DOUTPUT, picomotor_controller_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "driver_name", MXFT_STRING, NULL, \
	  			1, {MXU_PICOMOTOR_DRIVER_NAME_LENGTH},\
	MXF_REC_TYPE_STRUCT, offsetof(MX_PICOMOTOR_DOUTPUT, driver_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "channel_number", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PICOMOTOR_DOUTPUT, channel_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/* Define all of the interface functions. */

/* First the input functions. */

MX_API mx_status_type mxd_picomotor_din_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_picomotor_din_read( MX_DIGITAL_INPUT *dinput );

extern MX_RECORD_FUNCTION_LIST mxd_picomotor_din_record_function_list;
extern MX_DIGITAL_INPUT_FUNCTION_LIST
				mxd_picomotor_din_digital_input_function_list;

extern long mxd_picomotor_din_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_picomotor_din_rfield_def_ptr;

/* Second the output functions. */

MX_API mx_status_type mxd_picomotor_dout_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_picomotor_dout_read( MX_DIGITAL_OUTPUT *doutput );
MX_API mx_status_type mxd_picomotor_dout_write( MX_DIGITAL_OUTPUT *doutput );

extern MX_RECORD_FUNCTION_LIST mxd_picomotor_dout_record_function_list;
extern MX_DIGITAL_OUTPUT_FUNCTION_LIST
				mxd_picomotor_dout_digital_output_function_list;

extern long mxd_picomotor_dout_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_picomotor_dout_rfield_def_ptr;

#endif /* __D_PICOMOTOR_DIO_H__ */

