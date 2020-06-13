/* 
 * Name:    d_dante_mca.h
 *
 * Purpose: MCA driver for the XGLab DANTE multichannel analyzers.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2020 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_DANTE_MCA_H__
#define __D_DANTE_MCA_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "mx_mca.h"

#define MXU_DANTE_MCA_CHANNEL_NAME_LENGTH	80

typedef struct {
	MX_RECORD *record;

	MX_RECORD *dante_record;
	char channel_name[MXU_DANTE_MCA_CHANNEL_NAME_LENGTH+1];

	MX_RECORD *child_mcs_record;

	long mca_record_array_index;

#ifdef __cplusplus
	struct configuration *configuration;
#endif

} MX_DANTE_MCA;

#define MXD_DANTE_MCA_STANDARD_FIELDS \
  {-1, -1, "dante_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_DANTE_MCA, dante_record ), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "channel_name", MXFT_STRING, NULL, \
	  		1, {MXU_DANTE_MCA_CHANNEL_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_DANTE_MCA, channel_name ), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

MX_API mx_status_type mxd_dante_mca_initialize_driver( MX_DRIVER *driver );
MX_API mx_status_type mxd_dante_mca_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_dante_mca_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_dante_mca_delete_record( MX_RECORD *record );
MX_API mx_status_type mxd_dante_mca_print_structure( FILE *file,
							MX_RECORD *record );
MX_API mx_status_type mxd_dante_mca_open( MX_RECORD *record );
MX_API mx_status_type mxd_dante_mca_close( MX_RECORD *record );
MX_API mx_status_type mxd_dante_mca_special_processing_setup(
							MX_RECORD *record );

MX_API mx_status_type mxd_dante_mca_arm( MX_MCA *mca );
MX_API mx_status_type mxd_dante_mca_trigger( MX_MCA *mca );
MX_API mx_status_type mxd_dante_mca_stop( MX_MCA *mca );
MX_API mx_status_type mxd_dante_mca_read( MX_MCA *mca );
MX_API mx_status_type mxd_dante_mca_clear( MX_MCA *mca );
MX_API mx_status_type mxd_dante_mca_busy( MX_MCA *mca );
MX_API mx_status_type mxd_dante_mca_get_parameter( MX_MCA *mca );
MX_API mx_status_type mxd_dante_mca_set_parameter( MX_MCA *mca );

extern MX_RECORD_FUNCTION_LIST mxd_dante_mca_record_function_list;
extern MX_MCA_FUNCTION_LIST mxd_dante_mca_mca_function_list;

extern long mxd_dante_mca_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_dante_mca_rfield_def_ptr;

#ifdef __cplusplus
}
#endif

#endif /* __D_DANTE_MCA_H__ */
