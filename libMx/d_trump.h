/* 
 * Name:    d_trump.h
 *
 * Purpose: Header file for MX driver for the EG&G Ortec Trump MCA.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999-2000, 2006, 2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_TRUMP_H__
#define __D_TRUMP_H__

#include "mx_mca.h"

#define MXD_TRUMP_TICKS_PER_SECOND	(50.0)

typedef struct {
	MX_RECORD *umcbi_record;
	long detector_number;
	MX_UMCBI_DETECTOR *detector;
} MX_TRUMP_MCA;

#define MXD_TRUMP_STANDARD_FIELDS \
  {-1, -1, "umcbi_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_TRUMP_MCA, umcbi_record ), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "detector_number", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_TRUMP_MCA, detector_number ), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

MX_API mx_status_type mxd_trump_initialize_type( long type );
MX_API mx_status_type mxd_trump_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxd_trump_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_trump_print_structure( FILE *file,
							MX_RECORD *record );
MX_API mx_status_type mxd_trump_open( MX_RECORD *record );
MX_API mx_status_type mxd_trump_close( MX_RECORD *record );

MX_API mx_status_type mxd_trump_start( MX_MCA *mca );
MX_API mx_status_type mxd_trump_stop( MX_MCA *mca );
MX_API mx_status_type mxd_trump_read( MX_MCA *mca );
MX_API mx_status_type mxd_trump_clear( MX_MCA *mca );
MX_API mx_status_type mxd_trump_busy( MX_MCA *mca );
MX_API mx_status_type mxd_trump_get_parameter( MX_MCA *mca );
MX_API mx_status_type mxd_trump_set_parameter( MX_MCA *mca );

extern MX_RECORD_FUNCTION_LIST mxd_trump_record_function_list;
extern MX_MCA_FUNCTION_LIST mxd_trump_mca_function_list;

extern long mxd_trump_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_trump_rfield_def_ptr;

#endif /* __D_TRUMP_H__ */
