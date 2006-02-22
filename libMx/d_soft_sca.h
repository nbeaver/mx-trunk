/* 
 * Name:    d_soft_sca.h
 *
 * Purpose: Header file for MX software-emulated single channel analyzers.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2002 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_SOFT_SCA_H__
#define __D_SOFT_SCA_H__

#include "mx_sca.h"

typedef struct {
	int dummy;
} MX_SOFT_SCA;

#define MXD_SOFT_SCA_STANDARD_FIELDS 

MX_API mx_status_type mxd_soft_sca_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_soft_sca_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_soft_sca_open( MX_RECORD *record );

MX_API mx_status_type mxd_soft_sca_get_parameter( MX_SCA *sca );
MX_API mx_status_type mxd_soft_sca_set_parameter( MX_SCA *sca );

extern MX_RECORD_FUNCTION_LIST mxd_soft_sca_record_function_list;
extern MX_SCA_FUNCTION_LIST mxd_soft_sca_sca_function_list;

extern long mxd_soft_sca_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_soft_sca_rfield_def_ptr;

#endif /* __D_SOFT_SCA_H__ */
