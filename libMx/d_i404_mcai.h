/*
 * Name:    d_i404_mcai.h
 *
 * Purpose: Header file for MX driver to read out all 4 channels from a
 *          Pyramid Technical Consultants I404 digital electrometer at once.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_I404_MCAI_H__
#define __D_I404_MCAI_H__

#define MXU_I404_MCAI_TYPE_NAME_LENGTH	20
#define MXU_I404_MCAI_COMMAND_LENGTH	40

#define MXT_I404_MCAI_CHARGE		1
#define MXT_I404_MCAI_CURRENT		2
#define MXT_I404_MCAI_DIGITAL		3
#define MXT_I404_MCAI_HIGH_VOLTAGE	4
#define MXT_I404_MCAI_POSITION		5

typedef struct {
	MX_RECORD *record;

	MX_RECORD *i404_record;
	char mcai_type_name[ MXU_I404_MCAI_TYPE_NAME_LENGTH+1 ];

	long mcai_type;
	char command[ MXU_I404_MCAI_COMMAND_LENGTH+1 ];
} MX_I404_MCAI;

#define MXD_I404_MCAI_STANDARD_FIELDS \
  {-1, -1, "i404_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_I404_MCAI, i404_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "mcai_type_name", MXFT_STRING, NULL, \
				1, {MXU_I404_MCAI_TYPE_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_I404_MCAI, mcai_type_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

MX_API mx_status_type mxd_i404_mcai_initialize_driver( MX_DRIVER *driver );
MX_API mx_status_type mxd_i404_mcai_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_i404_mcai_finish_record_initialization(
							MX_RECORD *record );

MX_API mx_status_type mxd_i404_mcai_read( MX_MCAI *mcai );

extern MX_RECORD_FUNCTION_LIST mxd_i404_mcai_record_function_list;
extern MX_MCAI_FUNCTION_LIST mxd_i404_mcai_mcai_function_list;

extern long mxd_i404_mcai_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_i404_mcai_rfield_def_ptr;

#endif /* __D_I404_MCAI_H__ */

