/*
 * Name:    d_pmc_mcapi_aio.h
 *
 * Purpose: Header file for the MX input driver to control
 *          Precision MicroControl MCAPI controlled analog I/O ports.
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

#ifndef __D_PMC_MCAPI_AIO_H__
#define __D_PMC_MCAPI_AIO_H__

#include "mx_analog_input.h"
#include "mx_analog_output.h"

/* ===== PMC MCAPI analog input/output register data structures ===== */

typedef struct {
	MX_RECORD *record;

	MX_RECORD *pmc_mcapi_record;
	long channel_number;
} MX_PMC_MCAPI_AINPUT;

typedef struct {
	MX_RECORD *record;

	MX_RECORD *pmc_mcapi_record;
	long channel_number;
} MX_PMC_MCAPI_AOUTPUT;

#define MXD_PMC_MCAPI_AINPUT_STANDARD_FIELDS \
  {-1, -1, "pmc_mcapi_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PMC_MCAPI_AINPUT, pmc_mcapi_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "channel_number", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PMC_MCAPI_AINPUT, channel_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#define MXD_PMC_MCAPI_AOUTPUT_STANDARD_FIELDS \
  {-1, -1, "pmc_mcapi_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PMC_MCAPI_AOUTPUT, pmc_mcapi_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "channel_number", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PMC_MCAPI_AOUTPUT, channel_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/* Define all of the interface functions. */

/* First the input functions. */

MX_API mx_status_type mxd_pmc_mcapi_ain_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_pmc_mcapi_ain_read( MX_ANALOG_INPUT *ainput );

extern MX_RECORD_FUNCTION_LIST mxd_pmc_mcapi_ain_record_function_list;
extern MX_ANALOG_INPUT_FUNCTION_LIST
				mxd_pmc_mcapi_ain_analog_input_function_list;

extern long mxd_pmc_mcapi_ain_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_pmc_mcapi_ain_rfield_def_ptr;

/* Second the output functions. */

MX_API mx_status_type mxd_pmc_mcapi_aout_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_pmc_mcapi_aout_read( MX_ANALOG_OUTPUT *aoutput );
MX_API mx_status_type mxd_pmc_mcapi_aout_write( MX_ANALOG_OUTPUT *aoutput );

extern MX_RECORD_FUNCTION_LIST mxd_pmc_mcapi_aout_record_function_list;
extern MX_ANALOG_OUTPUT_FUNCTION_LIST
				mxd_pmc_mcapi_aout_analog_output_function_list;

extern long mxd_pmc_mcapi_aout_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_pmc_mcapi_aout_rfield_def_ptr;

#endif /* __D_PMC_MCAPI_AIO_H__ */

