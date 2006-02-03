/*
 * Name:    d_qbpm_mcai.h
 *
 * Purpose: Header file for MX driver to read out all 4 channels from an
 *          Oxford Danfysik QBPM beam position monitor at once.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_QBPM_MCAI_H__
#define __D_QBPM_MCAI_H__

typedef struct {
	MX_RECORD *record;

	MX_RECORD *icplus_record;

	char command[ MXU_ICPLUS_COMMAND_LENGTH+1 ];
} MX_QBPM_MCAI;

#define MXD_QBPM_MCAI_STANDARD_FIELDS \
  {-1, -1, "icplus_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_QBPM_MCAI, icplus_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

MX_API mx_status_type mxd_qbpm_mcai_initialize_type( long record_type );
MX_API mx_status_type mxd_qbpm_mcai_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_qbpm_mcai_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_qbpm_mcai_open( MX_RECORD *record );

MX_API mx_status_type mxd_qbpm_mcai_read( MX_MCAI *mcai );

extern MX_RECORD_FUNCTION_LIST mxd_qbpm_mcai_record_function_list;
extern MX_MCAI_FUNCTION_LIST mxd_qbpm_mcai_mcai_function_list;

extern mx_length_type mxd_qbpm_mcai_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_qbpm_mcai_rfield_def_ptr;

#endif /* __D_QBPM_MCAI_H__ */

