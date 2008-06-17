/*
 * Name:    d_spellman_df3_dio.h
 *
 * Purpose: Header file for MX digital I/O drivers for the Spellman
 *          DF3/FF3 series of high voltage power supplies.
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

#ifndef __D_SPELLMAN_DF3_DIO_H__
#define __D_SPELLMAN_DF3_DIO_H__

/* ===== Data structures ===== */

typedef struct {
	MX_RECORD *record;

	MX_RECORD *spellman_df3_record;
	unsigned long input_type;
} MX_SPELLMAN_DF3_DINPUT;

typedef struct {
	MX_RECORD *record;

	MX_RECORD *spellman_df3_record;
	unsigned long output_type;
} MX_SPELLMAN_DF3_DOUTPUT;

#define MXD_SPELLMAN_DF3_DINPUT_STANDARD_FIELDS \
  {-1, -1, "spellman_df3_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_SPELLMAN_DF3_DINPUT, spellman_df3_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "input_type", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SPELLMAN_DF3_DINPUT, input_type), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#define MXD_SPELLMAN_DF3_DOUTPUT_STANDARD_FIELDS \
  {-1, -1, "spellman_df3_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_SPELLMAN_DF3_DOUTPUT, spellman_df3_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "output_type", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SPELLMAN_DF3_DOUTPUT, output_type), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/* Define all of the interface functions. */

/* First the input functions. */

MX_API mx_status_type mxd_spellman_df3_din_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxd_spellman_df3_din_read( MX_DIGITAL_INPUT *dinput );

extern MX_RECORD_FUNCTION_LIST mxd_spellman_df3_din_record_function_list;
extern MX_DIGITAL_INPUT_FUNCTION_LIST
				mxd_spellman_df3_din_digital_input_function_list;

extern long mxd_spellman_df3_din_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_spellman_df3_din_rfield_def_ptr;

/* Second the output functions. */

MX_API mx_status_type mxd_spellman_df3_dout_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxd_spellman_df3_dout_read( MX_DIGITAL_OUTPUT *doutput );
MX_API mx_status_type mxd_spellman_df3_dout_write( MX_DIGITAL_OUTPUT *doutput );

extern MX_RECORD_FUNCTION_LIST mxd_spellman_df3_dout_record_function_list;
extern MX_DIGITAL_OUTPUT_FUNCTION_LIST
			mxd_spellman_df3_dout_digital_output_function_list;

extern long mxd_spellman_df3_dout_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_spellman_df3_dout_rfield_def_ptr;

#endif /* __D_SPELLMAN_DF3_DIO_H__ */

