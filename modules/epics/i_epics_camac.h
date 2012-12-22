/*
 * Name:    i_epics_camac.h
 *
 * Purpose: Header for MX driver for the EPICS CAMAC record.
 *
 *          At present, there is no support for block mode CAMAC operations
 *          in this driver.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2012 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_EPICS_CAMAC_H__
#define __I_EPICS_CAMAC_H__

MX_API mx_status_type mxi_epics_camac_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxi_epics_camac_finish_record_initialization(
							MX_RECORD *record );

MX_API mx_status_type mxi_epics_camac_get_lam_status( MX_CAMAC *crate,
							long *lam_status );

MX_API mx_status_type mxi_epics_camac_controller_command( MX_CAMAC *crate,
							long command );

MX_API mx_status_type mxi_epics_camac( MX_CAMAC *crate,
		long slot, long subaddress, long function_code,
		int32_t *data, int *Q, int *X );

typedef struct {
	MX_RECORD *record;

	char epics_record_name[ MXU_EPICS_PVNAME_LENGTH+1 ];

	MX_EPICS_PV a_pv;
	MX_EPICS_PV b_pv;
	MX_EPICS_PV c_pv;
	MX_EPICS_PV ccmd_pv;
	MX_EPICS_PV f_pv;
	MX_EPICS_PV n_pv;
	MX_EPICS_PV q_pv;
	MX_EPICS_PV tmod_pv;
	MX_EPICS_PV val_pv;
	MX_EPICS_PV x_pv;
} MX_EPICS_CAMAC;

extern MX_RECORD_FUNCTION_LIST mxi_epics_camac_record_function_list;
extern MX_CAMAC_FUNCTION_LIST mxi_epics_camac_camac_function_list;

extern long mxi_epics_camac_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_epics_camac_rfield_def_ptr;

#define MXI_EPICS_CAMAC_STANDARD_FIELDS \
  {-1, -1, "epics_record_name", MXFT_STRING, \
		NULL, 1, {MXU_EPICS_PVNAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EPICS_CAMAC, epics_record_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __I_EPICS_CAMAC_H__ */

