/*
 * Name:    i_soft_camac.h
 *
 * Purpose: Header for a MX driver for a software emulated CAMAC interface.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001-2002 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_SOFT_CAMAC_H__
#define __I_SOFT_CAMAC_H__

/* Define all of the interface functions. */

MX_API mx_status_type mxi_scamac_initialize_type( long type );
MX_API mx_status_type mxi_scamac_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxi_scamac_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxi_scamac_delete_record( MX_RECORD *record );
MX_API mx_status_type mxi_scamac_read_parms_from_hardware( MX_RECORD *record );
MX_API mx_status_type mxi_scamac_write_parms_to_hardware( MX_RECORD *record );
MX_API mx_status_type mxi_scamac_open( MX_RECORD *record );
MX_API mx_status_type mxi_scamac_close( MX_RECORD *record );
MX_API mx_status_type mxi_scamac_get_lam_status( MX_CAMAC *crate, int *lam_n );
MX_API mx_status_type mxi_scamac_controller_command( MX_CAMAC *crate,
								int command );
MX_API mx_status_type mxi_scamac_camac( MX_CAMAC *crate,
		int slot, int subaddress, int function_code,
		mx_sint32_type *data, int *Q, int *X );

#define SCAMAC_TEST_X	2
#define SCAMAC_TEST_Y	4
#define SCAMAC_TEST_Z	3

typedef struct {
	FILE *logfile;
	char logfile_name[256];
} MX_SCAMAC;

extern MX_RECORD_FUNCTION_LIST mxi_scamac_record_function_list;
extern MX_CAMAC_FUNCTION_LIST mxi_scamac_camac_function_list;

extern long mxi_scamac_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_scamac_record_field_def_ptr;

#define MXI_SCAMAC_STANDARD_FIELDS \
  {-1, -1, "logfile_name", MXFT_STRING, NULL, 1, {MXU_FILENAME_LENGTH+1}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SCAMAC, logfile_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __I_SOFT_CAMAC_H__ */

