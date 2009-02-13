/*
 * Name:    d_sd_m107.h
 *
 * Purpose: Header file for the Systron-Donner M107 DC voltage source.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_SD_M107_H__
#define __D_SD_M107_H__

typedef struct {
	MX_RECORD *gpib_record;
	long address;
} MX_SD_M107;

#define MXD_SD_M107_STANDARD_FIELDS \
  {-1, -1, "gpib_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SD_M107, gpib_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "address", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SD_M107, address), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/* Define all of the interface functions. */

MX_API mx_status_type mxd_sd_m107_create_record_structures( MX_RECORD *record );

MX_API mx_status_type mxd_sd_m107_write( MX_ANALOG_OUTPUT *dac );

extern MX_RECORD_FUNCTION_LIST mxd_sd_m107_record_function_list;
extern MX_ANALOG_OUTPUT_FUNCTION_LIST mxd_sd_m107_analog_output_function_list;

extern long mxd_sd_m107_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_sd_m107_rfield_def_ptr;

#endif /* __D_SD_M107_H__ */

