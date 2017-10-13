/* 
 * Name:    d_soft_mca.h
 *
 * Purpose: Header file for MX software-emulated multichannel analyzers.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2000-2001, 2010, 2012, 2017 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_SOFT_MCA_H__
#define __D_SOFT_MCA_H__

#include "mx_mca.h"

#define MXD_SOFT_MCA_MULTIPLIER_STEP	0.1

typedef struct {
	char filename[MXU_FILENAME_LENGTH + 1];
	double multiplier;
	double *simulated_channel_array;
	MX_CLOCK_TICK finish_time_in_clock_ticks;
} MX_SOFT_MCA;

#define MXD_SOFT_MCA_STANDARD_FIELDS \
  {-1, -1, "filename", MXFT_STRING, NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SOFT_MCA, filename), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

MX_API mx_status_type mxd_soft_mca_initialize_driver( MX_DRIVER *driver );
MX_API mx_status_type mxd_soft_mca_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_soft_mca_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_soft_mca_delete_record( MX_RECORD *record );
MX_API mx_status_type mxd_soft_mca_print_structure( FILE *file,
							MX_RECORD *record );
MX_API mx_status_type mxd_soft_mca_open( MX_RECORD *record );

MX_API mx_status_type mxd_soft_mca_trigger( MX_MCA *mca );
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
