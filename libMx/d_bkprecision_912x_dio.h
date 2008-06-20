/*
 * Name:    d_bkprecision_912x_dio.h
 *
 * Purpose: Header file for MX digital I/O drivers for the BK Precision 912x
 *          series of power supplies.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_BKPRECISION_912X_DIO_H__
#define __D_BKPRECISION_912X_DIO_H__

/* ===== Data structures ===== */

typedef struct {
	MX_RECORD *record;

	MX_RECORD *bkprecision_912x_record;
} MX_BKPRECISION_912X_DINPUT;

typedef struct {
	MX_RECORD *record;

	MX_RECORD *bkprecision_912x_record;
} MX_BKPRECISION_912X_DOUTPUT;

#define MXD_BKPRECISION_912X_DINPUT_STANDARD_FIELDS \
  {-1, -1, "bkprecision_912x_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_BKPRECISION_912X_DINPUT, bkprecision_912x_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#define MXD_BKPRECISION_912X_DOUTPUT_STANDARD_FIELDS \
  {-1, -1, "bkprecision_912x_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_BKPRECISION_912X_DOUTPUT, bkprecision_912x_record),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/* Define all of the interface functions. */

/* First the input functions. */

MX_API mx_status_type mxd_bkprecision_912x_din_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxd_bkprecision_912x_din_read( MX_DIGITAL_INPUT *dinput );

extern MX_RECORD_FUNCTION_LIST mxd_bkprecision_912x_din_record_function_list;
extern MX_DIGITAL_INPUT_FUNCTION_LIST
			mxd_bkprecision_912x_din_digital_input_function_list;

extern long mxd_bkprecision_912x_din_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_bkprecision_912x_din_rfield_def_ptr;

/* Second the output functions. */

MX_API mx_status_type mxd_bkprecision_912x_dout_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxd_bkprecision_912x_dout_read(
						MX_DIGITAL_OUTPUT *doutput );
MX_API mx_status_type mxd_bkprecision_912x_dout_write(
						MX_DIGITAL_OUTPUT *doutput );

extern MX_RECORD_FUNCTION_LIST mxd_bkprecision_912x_dout_record_function_list;
extern MX_DIGITAL_OUTPUT_FUNCTION_LIST
			mxd_bkprecision_912x_dout_digital_output_function_list;

extern long mxd_bkprecision_912x_dout_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_bkprecision_912x_dout_rfield_def_ptr;

#endif /* __D_BKPRECISION_912X_DIO_H__ */

