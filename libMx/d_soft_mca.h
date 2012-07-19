/* 
 * Name:    d_soft_mca.h
 *
 * Purpose: Header file for MX software-emulated multichannel analyzers
 *          using simple Monte Carlo calculations.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2000-2001, 2010, 2012 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_SOFT_MCA_H__
#define __D_SOFT_MCA_H__

#include "mx_mca.h"

#define MXU_SOFT_MCA_MAX_SOURCE_STRING_LENGTH	80

typedef struct {
	long num_sources;
	char **source_string_array;

	MX_CLOCK_TICK finish_time_in_clock_ticks;
} MX_SOFT_MCA;

#define MXD_SOFT_MCA_STANDARD_FIELDS \
  {-1, -1, "num_sources", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SOFT_MCA, num_sources), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "source_string_array", MXFT_STRING, NULL, \
	    2, {MXU_VARARGS_LENGTH, MXU_SOFT_MCA_MAX_SOURCE_STRING_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SOFT_MCA, source_string_array), \
	{sizeof(char), sizeof(char *)}, NULL, \
	    (MXFF_IN_DESCRIPTION | MXFF_READ_ONLY | MXFF_VARARGS) }

MX_API mx_status_type mxd_soft_mca_initialize_driver( MX_DRIVER *driver );
MX_API mx_status_type mxd_soft_mca_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_soft_mca_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_soft_mca_delete_record( MX_RECORD *record );
MX_API mx_status_type mxd_soft_mca_print_structure( FILE *file,
							MX_RECORD *record );
MX_API mx_status_type mxd_soft_mca_open( MX_RECORD *record );

MX_API mx_status_type mxd_soft_mca_start( MX_MCA *mca );
MX_API mx_status_type mxd_soft_mca_stop( MX_MCA *mca );
MX_API mx_status_type mxd_soft_mca_read( MX_MCA *mca );
MX_API mx_status_type mxd_soft_mca_clear( MX_MCA *mca );
MX_API mx_status_type mxd_soft_mca_busy( MX_MCA *mca );
MX_API mx_status_type mxd_soft_mca_get_parameter( MX_MCA *mca );

extern MX_RECORD_FUNCTION_LIST mxd_soft_mca_record_function_list;
extern MX_MCA_FUNCTION_LIST mxd_soft_mca_mca_function_list;

extern long mxd_soft_mca_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_soft_mca_rfield_def_ptr;

#endif /* __D_SOFT_MCA_H__ */
