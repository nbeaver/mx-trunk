/*
 * Name:    d_pmc_mcapi_dio.h
 *
 * Purpose: Header file for MX input and output drivers to control 
 *          Precision MicroControl MCAPI digital I/O ports.
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

#ifndef __D_PMC_MCAPI_DIO_H__
#define __D_PMC_MCAPI_DIO_H__

/* Values for the 'pmc_mcapi_dinput_flags' and 'pmc_mcapi_doutput_flags' fields.
 */

#define MXF_PMC_MCAPI_DIO_HIGH	0x4
#define MXF_PMC_MCAPI_DIO_LOW	0x8
#define MXF_PMC_MCAPI_DIO_LATCH	0x10

/* ===== PMC MCAPI digital input/output register data structures ===== */

typedef struct {
	MX_RECORD *record;

	MX_RECORD *pmc_mcapi_record;
	unsigned short channel_number;
	unsigned long pmc_mcapi_dinput_flags;
} MX_PMC_MCAPI_DINPUT;

typedef struct {
	MX_RECORD *record;

	MX_RECORD *pmc_mcapi_record;
	unsigned short channel_number;
	unsigned long pmc_mcapi_doutput_flags;
} MX_PMC_MCAPI_DOUTPUT;

#define MXD_PMC_MCAPI_DINPUT_STANDARD_FIELDS \
  {-1, -1, "pmc_mcapi_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_PMC_MCAPI_DINPUT, pmc_mcapi_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "channel_number", MXFT_USHORT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PMC_MCAPI_DINPUT, channel_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "pmc_mcapi_dinput_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_PMC_MCAPI_DINPUT, pmc_mcapi_dinput_flags), \
	{0}, NULL, MXFF_IN_DESCRIPTION}

#define MXD_PMC_MCAPI_DOUTPUT_STANDARD_FIELDS \
  {-1, -1, "pmc_mcapi_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_PMC_MCAPI_DOUTPUT, pmc_mcapi_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "channel_number", MXFT_USHORT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PMC_MCAPI_DOUTPUT, channel_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "pmc_mcapi_doutput_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_PMC_MCAPI_DOUTPUT, pmc_mcapi_doutput_flags), \
	{0}, NULL, MXFF_IN_DESCRIPTION}

/* Define all of the interface functions. */

/* First the input functions. */

MX_API mx_status_type mxd_pmc_mcapi_din_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_pmc_mcapi_din_open( MX_RECORD *record );

MX_API mx_status_type mxd_pmc_mcapi_din_read( MX_DIGITAL_INPUT *dinput );

extern MX_RECORD_FUNCTION_LIST mxd_pmc_mcapi_din_record_function_list;
extern MX_DIGITAL_INPUT_FUNCTION_LIST
				mxd_pmc_mcapi_din_digital_input_function_list;

extern long mxd_pmc_mcapi_din_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_pmc_mcapi_din_rfield_def_ptr;

/* Second the output functions. */

MX_API mx_status_type mxd_pmc_mcapi_dout_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_pmc_mcapi_dout_open( MX_RECORD *record );

MX_API mx_status_type mxd_pmc_mcapi_dout_read( MX_DIGITAL_OUTPUT *doutput );
MX_API mx_status_type mxd_pmc_mcapi_dout_write( MX_DIGITAL_OUTPUT *doutput );

extern MX_RECORD_FUNCTION_LIST mxd_pmc_mcapi_dout_record_function_list;
extern MX_DIGITAL_OUTPUT_FUNCTION_LIST
				mxd_pmc_mcapi_dout_digital_output_function_list;

extern long mxd_pmc_mcapi_dout_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_pmc_mcapi_dout_rfield_def_ptr;

#endif /* __D_PMC_MCAPI_DIO_H__ */

