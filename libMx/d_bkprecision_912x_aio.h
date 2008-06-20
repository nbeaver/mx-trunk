/*
 * Name:    d_bkprecision_912x_aio.h
 *
 * Purpose: Header file for MX analog I/O drivers for the BK Precision 912x
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

#ifndef __D_BKPRECISION_912X_AIO_H__
#define __D_BKPRECISION_912X_AIO_H__

/*--- Input and output types ---*/

#define MXT_BKPRECISION_912X_VOLTAGE	1
#define MXT_BKPRECISION_912X_CURRENT	2

/*--- Input types only ---*/

#define MXT_BKPRECISION_912X_POWER	3
#define MXT_BKPRECISION_912X_DVM	4
#define MXT_BKPRECISION_912X_RESISTANCE	5

/* ===== Data structures ===== */

typedef struct {
	MX_RECORD *record;

	MX_RECORD *bkprecision_912x_record;
	char input_type_name[MXU_BKPRECISION_NAME_LENGTH+1];

	unsigned long input_type;
} MX_BKPRECISION_912X_AINPUT;

typedef struct {
	MX_RECORD *record;

	MX_RECORD *bkprecision_912x_record;
	char output_type_name[MXU_BKPRECISION_NAME_LENGTH+1];

	unsigned long output_type;
} MX_BKPRECISION_912X_AOUTPUT;

#define MXD_BKPRECISION_912X_AINPUT_STANDARD_FIELDS \
  {-1, -1, "bkprecision_912x_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_BKPRECISION_912X_AINPUT, bkprecision_912x_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "input_type_name", MXFT_STRING, NULL, \
					1, {MXU_BKPRECISION_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_BKPRECISION_912X_AINPUT, input_type_name),\
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#define MXD_BKPRECISION_912X_AOUTPUT_STANDARD_FIELDS \
  {-1, -1, "bkprecision_912x_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_BKPRECISION_912X_AOUTPUT, bkprecision_912x_record),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "output_type_name", MXFT_STRING, NULL, \
					1, {MXU_BKPRECISION_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_BKPRECISION_912X_AOUTPUT, output_type_name),\
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/* Define all of the interface functions. */

/* First the input functions. */

MX_API mx_status_type mxd_bkprecision_912x_ain_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_bkprecision_912x_ain_open( MX_RECORD *record );

MX_API mx_status_type mxd_bkprecision_912x_ain_read( MX_ANALOG_INPUT *ainput );

extern MX_RECORD_FUNCTION_LIST mxd_bkprecision_912x_ain_record_function_list;
extern MX_ANALOG_INPUT_FUNCTION_LIST
			mxd_bkprecision_912x_ain_analog_input_function_list;

extern long mxd_bkprecision_912x_ain_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_bkprecision_912x_ain_rfield_def_ptr;

/* Second the output functions. */

MX_API mx_status_type mxd_bkprecision_912x_aout_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_bkprecision_912x_aout_open( MX_RECORD *record );

MX_API mx_status_type mxd_bkprecision_912x_aout_read(
						MX_ANALOG_OUTPUT *aoutput );
MX_API mx_status_type mxd_bkprecision_912x_aout_write(
						MX_ANALOG_OUTPUT *aoutput );

extern MX_RECORD_FUNCTION_LIST mxd_bkprecision_912x_aout_record_function_list;
extern MX_ANALOG_OUTPUT_FUNCTION_LIST
			mxd_bkprecision_912x_aout_analog_output_function_list;

extern long mxd_bkprecision_912x_aout_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_bkprecision_912x_aout_rfield_def_ptr;

#endif /* __D_BKPRECISION_912X_AIO_H__ */

