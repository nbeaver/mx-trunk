/*
 * Name:    d_tracker_dio.h
 *
 * Purpose: Header file for drivers to control Data Track Tracker locations
 *          as digital I/O ports.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_TRACKER_DIO_H__
#define __D_TRACKER_DIO_H__

typedef struct {
	MX_RECORD *record;

	MX_RECORD *rs232_record;
	int address;
	int location;
} MX_TRACKER_DINPUT;

typedef struct {
	MX_RECORD *record;

	MX_RECORD *rs232_record;
	int address;
	int location;
} MX_TRACKER_DOUTPUT;

#define MXD_TRACKER_DINPUT_STANDARD_FIELDS \
  {-1, -1, "rs232_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_TRACKER_DINPUT, rs232_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "address", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_TRACKER_DINPUT, address), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "location", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_TRACKER_DINPUT, location), \
	{0}, NULL, MXFF_IN_DESCRIPTION}

#define MXD_TRACKER_DOUTPUT_STANDARD_FIELDS \
  {-1, -1, "rs232_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_TRACKER_DOUTPUT, rs232_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "address", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_TRACKER_DOUTPUT, address), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "location", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_TRACKER_DOUTPUT, location), \
	{0}, NULL, MXFF_IN_DESCRIPTION}

/* Define all of the interface functions. */

/* First the input functions. */

MX_API mx_status_type mxd_tracker_din_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_tracker_din_finish_record_initialization(
							MX_RECORD *record );

MX_API mx_status_type mxd_tracker_din_read( MX_DIGITAL_INPUT *dinput );

extern MX_RECORD_FUNCTION_LIST mxd_tracker_din_record_function_list;
extern MX_DIGITAL_INPUT_FUNCTION_LIST
				mxd_tracker_din_digital_input_function_list;

extern mx_length_type mxd_tracker_din_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_tracker_din_rfield_def_ptr;

/* Second the output functions. */

MX_API mx_status_type mxd_tracker_dout_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_tracker_dout_finish_record_initialization(
							MX_RECORD *record );

MX_API mx_status_type mxd_tracker_dout_read( MX_DIGITAL_OUTPUT *doutput );
MX_API mx_status_type mxd_tracker_dout_write( MX_DIGITAL_OUTPUT *doutput );

extern MX_RECORD_FUNCTION_LIST mxd_tracker_dout_record_function_list;
extern MX_DIGITAL_OUTPUT_FUNCTION_LIST
				mxd_tracker_dout_digital_output_function_list;

extern mx_length_type mxd_tracker_dout_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_tracker_dout_rfield_def_ptr;

#endif /* __D_TRACKER_DIO_H__ */

