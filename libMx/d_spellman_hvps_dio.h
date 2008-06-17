/*
 * Name:    d_spellman_hvps_dio.h
 *
 * Purpose: Header file for MX digital I/O drivers for the Spellman
 *          high voltage power supply.
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

#ifndef __D_SPELLMAN_HVPS_DIO_H__
#define __D_SPELLMAN_HVPS_DIO_H__

/* ===== Data structures ===== */

typedef struct {
	MX_RECORD *record;

	MX_RECORD *spellman_hvps_record;
	unsigned long input_type;
} MX_SPELLMAN_HVPS_DINPUT;

typedef struct {
	MX_RECORD *record;

	MX_RECORD *spellman_hvps_record;
	unsigned long output_type;
} MX_SPELLMAN_HVPS_DOUTPUT;

#define MXD_SPELLMAN_HVPS_DINPUT_STANDARD_FIELDS \
  {-1, -1, "spellman_hvps_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_SPELLMAN_HVPS_DINPUT, spellman_hvps_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "input_type", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SPELLMAN_HVPS_DINPUT, input_type), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#define MXD_SPELLMAN_HVPS_DOUTPUT_STANDARD_FIELDS \
  {-1, -1, "spellman_hvps_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_SPELLMAN_HVPS_DOUTPUT, spellman_hvps_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "output_type", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SPELLMAN_HVPS_DOUTPUT, output_type), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/* Define all of the interface functions. */

/* First the input functions. */

MX_API mx_status_type mxd_spellman_hvps_din_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxd_spellman_hvps_din_read( MX_DIGITAL_INPUT *dinput );

extern MX_RECORD_FUNCTION_LIST mxd_spellman_hvps_din_record_function_list;
extern MX_DIGITAL_INPUT_FUNCTION_LIST
				mxd_spellman_hvps_din_digital_input_function_list;

extern long mxd_spellman_hvps_din_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_spellman_hvps_din_rfield_def_ptr;

/* Second the output functions. */

MX_API mx_status_type mxd_spellman_hvps_dout_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxd_spellman_hvps_dout_read( MX_DIGITAL_OUTPUT *doutput );
MX_API mx_status_type mxd_spellman_hvps_dout_write( MX_DIGITAL_OUTPUT *doutput );

extern MX_RECORD_FUNCTION_LIST mxd_spellman_hvps_dout_record_function_list;
extern MX_DIGITAL_OUTPUT_FUNCTION_LIST
			mxd_spellman_hvps_dout_digital_output_function_list;

extern long mxd_spellman_hvps_dout_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_spellman_hvps_dout_rfield_def_ptr;

#endif /* __D_SPELLMAN_HVPS_DIO_H__ */

