/*
 * Name:    d_spellman_df3_aio.h
 *
 * Purpose: Header file for MX analog I/O drivers for the Spellman
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

#ifndef __D_SPELLMAN_DF3_AIO_H__
#define __D_SPELLMAN_DF3_AIO_H__

/* ===== Data structures ===== */

typedef struct {
	MX_RECORD *record;

	MX_RECORD *spellman_df3_record;
	unsigned long input_type;
} MX_SPELLMAN_DF3_AINPUT;

typedef struct {
	MX_RECORD *record;

	MX_RECORD *spellman_df3_record;
	unsigned long output_type;
} MX_SPELLMAN_DF3_AOUTPUT;

#define MXD_SPELLMAN_DF3_AINPUT_STANDARD_FIELDS \
  {-1, -1, "spellman_df3_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_SPELLMAN_DF3_AINPUT, spellman_df3_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "input_type", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SPELLMAN_DF3_AINPUT, input_type),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#define MXD_SPELLMAN_DF3_AOUTPUT_STANDARD_FIELDS \
  {-1, -1, "spellman_df3_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_SPELLMAN_DF3_AOUTPUT, spellman_df3_record),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "output_type", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SPELLMAN_DF3_AOUTPUT, output_type),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/* Define all of the interface functions. */

/* First the input functions. */

MX_API mx_status_type mxd_spellman_df3_ain_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxd_spellman_df3_ain_read( MX_ANALOG_INPUT *ainput );

extern MX_RECORD_FUNCTION_LIST mxd_spellman_df3_ain_record_function_list;
extern MX_ANALOG_INPUT_FUNCTION_LIST
			mxd_spellman_df3_ain_analog_input_function_list;

extern long mxd_spellman_df3_ain_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_spellman_df3_ain_rfield_def_ptr;

/* Second the output functions. */

MX_API mx_status_type mxd_spellman_df3_aout_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxd_spellman_df3_aout_read( MX_ANALOG_OUTPUT *aoutput );
MX_API mx_status_type mxd_spellman_df3_aout_write( MX_ANALOG_OUTPUT *aoutput );

extern MX_RECORD_FUNCTION_LIST mxd_spellman_df3_aout_record_function_list;
extern MX_ANALOG_OUTPUT_FUNCTION_LIST
			mxd_spellman_df3_aout_analog_output_function_list;

extern long mxd_spellman_df3_aout_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_spellman_df3_aout_rfield_def_ptr;

#endif /* __D_SPELLMAN_DF3_AIO_H__ */

