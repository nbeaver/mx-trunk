/*
 * Name:    d_mlfsom.h
 *
 * Purpose: Header file for MX area detector simulator using the mlfsom
 *          program written by James Holton of Lawrence Berkeley National
 *          Laboratory.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2009 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_MLFSOM_H__
#define __D_MLFSOM_H__

typedef struct {
	MX_RECORD *record;
	MX_AREA_DETECTOR *ad;
	struct mx_mlfsom_type *mlfsom;
	char command[500];
} MX_MLFSOM_THREAD_INFO;

typedef struct mx_mlfsom_type {
	MX_RECORD *record;

	char work_directory[MXU_FILENAME_LENGTH+1];
	char pdb_filename[MXU_FILENAME_LENGTH+1];
	char ano_sfall_filename[MXU_FILENAME_LENGTH+1];
	char mlfsom_filename[MXU_FILENAME_LENGTH+1];

	MX_THREAD *ano_sfall_thread;
	MX_THREAD *mlfsom_thread;

	MX_MLFSOM_THREAD_INFO *ano_sfall_tinfo;
	MX_MLFSOM_THREAD_INFO *mlfsom_tinfo;
} MX_MLFSOM;


#define MXD_MLFSOM_STANDARD_FIELDS \
  {-1, -1, "work_directory", MXFT_STRING, NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MLFSOM, work_directory), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "pdb_filename", MXFT_STRING, NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MLFSOM, pdb_filename), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "ano_sfall_filename", MXFT_STRING, NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MLFSOM, ano_sfall_filename), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "mlfsom_filename", MXFT_STRING, NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MLFSOM, mlfsom_filename), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION }

MX_API mx_status_type mxd_mlfsom_initialize_type( long record_type );
MX_API mx_status_type mxd_mlfsom_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxd_mlfsom_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_mlfsom_open( MX_RECORD *record );
MX_API mx_status_type mxd_mlfsom_close( MX_RECORD *record );

MX_API mx_status_type mxd_mlfsom_arm( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_mlfsom_trigger( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_mlfsom_stop( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_mlfsom_abort( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_mlfsom_get_extended_status( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_mlfsom_readout_frame( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_mlfsom_get_parameter( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_mlfsom_set_parameter( MX_AREA_DETECTOR *ad );

extern MX_RECORD_FUNCTION_LIST mxd_mlfsom_record_function_list;
extern MX_AREA_DETECTOR_FUNCTION_LIST
			mxd_mlfsom_ad_function_list;

extern long mxd_mlfsom_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_mlfsom_rfield_def_ptr;

#endif /* __D_MLFSOM_H__ */

