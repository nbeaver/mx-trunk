/*
 * Name:    d_handel_mapping_pixel_next.h
 *
 * Purpose: Header file for Handel digital output driver that sends
 *          'mapping_pixel_next' messages.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2019 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_HANDEL_MAPPING_PIXEL_NEXT_H__
#define __D_HANDEL_MAPPING_PIXEL_NEXT_H__

#include "mx_digital_output.h"

typedef struct {
	MX_RECORD *record;

	MX_RECORD *handel_record;
} MX_HANDEL_MAPPING_PIXEL_NEXT;

#define MXD_HANDEL_MAPPING_PIXEL_NEXT_STANDARD_FIELDS \
  {-1, -1, "handel_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof( MX_HANDEL_MAPPING_PIXEL_NEXT, handel_record ), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

/* Define all of the interface functions. */

MX_API mx_status_type mxd_handel_mpn_initialize_driver( MX_DRIVER *driver );
MX_API mx_status_type mxd_handel_mpn_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_handel_mpn_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_handel_mpn_delete_record( MX_RECORD *record );
MX_API mx_status_type mxd_handel_mpn_print_structure( FILE *file,
							MX_RECORD *record );
MX_API mx_status_type mxd_handel_mpn_open( MX_RECORD *record );
MX_API mx_status_type mxd_handel_mpn_close( MX_RECORD *record );

MX_API mx_status_type mxd_handel_mpn_read( MX_DIGITAL_OUTPUT *doutput );
MX_API mx_status_type mxd_handel_mpn_write( MX_DIGITAL_OUTPUT *doutput );

extern MX_RECORD_FUNCTION_LIST mxd_handel_mpn_record_function_list;
extern MX_DIGITAL_OUTPUT_FUNCTION_LIST
			mxd_handel_mpn_digital_output_function_list;

extern long mxd_handel_mpn_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_handel_mpn_rfield_def_ptr;

#endif /* __D_HANDEL_MAPPING_PIXEL_NEXT_H__ */

