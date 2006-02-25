/*
 * Name:    d_ks3112.h
 *
 * Purpose: Header file for MX output drivers to control KS3112 analog output.
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

#ifndef __D_KS3112_H__
#define __D_KS3112_H__

#include "mx_analog_output.h"

/* ===== KS3112 analog output register data structure ===== */

typedef struct {
	MX_RECORD *camac_record;
	long slot;
	long subaddress;
} MX_KS3112;

#define MXD_KS3112_STANDARD_FIELDS \
  {-1, -1, "camac_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_KS3112, camac_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "slot", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_KS3112, slot), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "subaddress", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_KS3112, subaddress), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/* Define all of the interface functions. */

MX_API mx_status_type mxd_ks3112_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxd_ks3112_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_ks3112_print_structure( FILE *file,
							MX_RECORD *record );
MX_API mx_status_type mxd_ks3112_open( MX_RECORD *record );

MX_API mx_status_type mxd_ks3112_read( MX_ANALOG_OUTPUT *dac );
MX_API mx_status_type mxd_ks3112_write( MX_ANALOG_OUTPUT *dac );

extern MX_RECORD_FUNCTION_LIST mxd_ks3112_record_function_list;
extern MX_ANALOG_OUTPUT_FUNCTION_LIST mxd_ks3112_analog_output_function_list;

extern long mxd_ks3112_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_ks3112_rfield_def_ptr;

#endif /* __D_KS3112_H__ */

