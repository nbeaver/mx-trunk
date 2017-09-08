/* 
 * Name:    d_amptek_dp5_mca.h
 *
 * Purpose: Header file for Amptek MCAs that use the DP5 protocol.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2017 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_AMPTEK_DP5_MCA_H__
#define __D_AMPTEK_DP5_MCA_H__

#define MXU_AMPTEK_DP5_MAX_SCAS	8
#define MXU_AMPTEK_DP5_MAX_BINS	8192

typedef struct {
	MX_RECORD *record;

	MX_RECORD *amptek_dp5_record;

	char raw_mca_spectrum[ 3 * MXU_AMPTEK_DP5_MAX_BINS ];
} MX_AMPTEK_DP5_MCA;

#define MXD_AMPTEK_DP5_STANDARD_FIELDS \
  {-1, -1, "amptek_dp5_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_AMPTEK_DP5_MCA, amptek_dp5_record ), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

MX_API mx_status_type mxd_amptek_dp5_mca_initialize_driver( MX_DRIVER *driver );
MX_API mx_status_type mxd_amptek_dp5_mca_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_amptek_dp5_mca_open( MX_RECORD *record );
MX_API mx_status_type mxd_amptek_dp5_mca_special_processing_setup(
							MX_RECORD *record );

MX_API mx_status_type mxd_amptek_dp5_mca_start( MX_MCA *mca );
MX_API mx_status_type mxd_amptek_dp5_mca_stop( MX_MCA *mca );
MX_API mx_status_type mxd_amptek_dp5_mca_read( MX_MCA *mca );
MX_API mx_status_type mxd_amptek_dp5_mca_clear( MX_MCA *mca );
MX_API mx_status_type mxd_amptek_dp5_mca_busy( MX_MCA *mca );
MX_API mx_status_type mxd_amptek_dp5_mca_get_parameter( MX_MCA *mca );
MX_API mx_status_type mxd_amptek_dp5_mca_set_parameter( MX_MCA *mca );

extern MX_RECORD_FUNCTION_LIST mxd_amptek_dp5_mca_record_function_list;
extern MX_MCA_FUNCTION_LIST mxd_amptek_dp5_mca_mca_function_list;

extern long mxd_amptek_dp5_mca_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_amptek_dp5_mca_rfield_def_ptr;

#endif /* __D_AMPTEK_DP5_MCA_H__ */
