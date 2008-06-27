/*
 * Name:    d_bkprecision_912x_wvout.h
 *
 * Purpose: Header for MX waveform output driver for the BK Precision
 *          912x series of power supplies.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_BKPRECISION_912X_WVOUT_H__
#define __D_BKPRECISION_912X_WVOUT_H__

/* Internal channel numbers for LIST:VOLTAGE, LIST:CURRENT, and LIST:WIDTH. */

#define MXF_BKPRECISION_912X_WVOUT_VOLTAGE	0
#define MXF_BKPRECISION_912X_WVOUT_CURRENT	1
#define MXF_BKPRECISION_912X_WVOUT_WIDTH	2

/* Values for 'list_mode' */

#define MXF_BKPRECISION_912X_WVOUT_CONTINUOUS	1
#define MXF_BKPRECISION_912X_WVOUT_STEP		2

/* Values for 'list_step' */

#define MXF_BKPRECISION_912X_WVOUT_ONCE		1
#define MXF_BKPRECISION_912X_WVOUT_REPEAT	2

/*----*/

typedef struct {
	MX_RECORD *record;

	MX_RECORD *bkprecision_912x_record;
	char list_mode_name[MXU_BKPRECISION_NAME_LENGTH+1];
	char list_step_name[MXU_BKPRECISION_NAME_LENGTH+1];

	unsigned long list_mode;
	unsigned long list_step;
} MX_BKPRECISION_912X_WVOUT;

#define MXD_BKPRECISION_912X_WVOUT_STANDARD_FIELDS \
  {-1, -1, "bkprecision_912x_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_BKPRECISION_912X_WVOUT, bkprecision_912x_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "list_mode_name", MXFT_STRING, NULL, \
					1, {MXU_BKPRECISION_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_BKPRECISION_912X_WVOUT, list_mode_name), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "list_step_name", MXFT_STRING, NULL, \
					1, {MXU_BKPRECISION_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_BKPRECISION_912X_WVOUT, list_step_name), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "list_mode", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_BKPRECISION_912X_WVOUT, list_mode), \
	{0}, NULL, 0}, \
  \
  {-1, -1, "list_step", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_BKPRECISION_912X_WVOUT, list_step), \
	{0}, NULL, 0}

MX_API mx_status_type mxd_bkprecision_912x_wvout_initialize_type( long type );
MX_API mx_status_type mxd_bkprecision_912x_wvout_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_bkprecision_912x_wvout_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_bkprecision_912x_wvout_open( MX_RECORD *record );

MX_API mx_status_type mxd_bkprecision_912x_wvout_arm(
						MX_WAVEFORM_OUTPUT *wvout );
MX_API mx_status_type mxd_bkprecision_912x_wvout_trigger(
						MX_WAVEFORM_OUTPUT *wvout );
MX_API mx_status_type mxd_bkprecision_912x_wvout_stop(
						MX_WAVEFORM_OUTPUT *wvout );
MX_API mx_status_type mxd_bkprecision_912x_wvout_busy(
						MX_WAVEFORM_OUTPUT *wvout );
MX_API mx_status_type mxd_bkprecision_912x_wvout_read_channel(
						MX_WAVEFORM_OUTPUT *wvout );
MX_API mx_status_type mxd_bkprecision_912x_wvout_write_channel(
						MX_WAVEFORM_OUTPUT *wvout );
MX_API mx_status_type mxd_bkprecision_912x_wvout_get_parameter(
						MX_WAVEFORM_OUTPUT *wvout );
MX_API mx_status_type mxd_bkprecision_912x_wvout_set_parameter(
						MX_WAVEFORM_OUTPUT *wvout );

extern MX_RECORD_FUNCTION_LIST mxd_bkprecision_912x_wvout_record_function_list;
extern MX_WAVEFORM_OUTPUT_FUNCTION_LIST
			mxd_bkprecision_912x_wvout_wvout_function_list;

extern long mxd_bkprecision_912x_wvout_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_bkprecision_912x_wvout_rfield_def_ptr;

#endif /* __D_BKPRECISION_912X_WVOUT_H__ */
