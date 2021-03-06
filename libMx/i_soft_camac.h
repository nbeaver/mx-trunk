/*
 * Name:    i_soft_camac.h
 *
 * Purpose: Header for a MX driver for a software emulated CAMAC interface.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001-2002, 2005-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_SOFT_CAMAC_H__
#define __I_SOFT_CAMAC_H__

/* Define all of the interface functions. */

MX_API mx_status_type mxi_scamac_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxi_scamac_finish_record_initialization(
							MX_RECORD *record );

MX_API mx_status_type mxi_scamac_get_lam_status( MX_CAMAC *crate, long *lam_n );
MX_API mx_status_type mxi_scamac_controller_command( MX_CAMAC *crate,
								long command );
MX_API mx_status_type mxi_scamac_camac( MX_CAMAC *crate,
		long slot, long subaddress, long function_code,
		int32_t *data, int *Q, int *X );

typedef struct {
	FILE *logfile;
	char logfile_name[MXU_FILENAME_LENGTH+1];
} MX_SCAMAC;

extern MX_RECORD_FUNCTION_LIST mxi_scamac_record_function_list;
extern MX_CAMAC_FUNCTION_LIST mxi_scamac_camac_function_list;

extern long mxi_scamac_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_scamac_record_field_def_ptr;

#define MXI_SCAMAC_STANDARD_FIELDS \
  {-1, -1, "logfile_name", MXFT_STRING, NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SCAMAC, logfile_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __I_SOFT_CAMAC_H__ */

