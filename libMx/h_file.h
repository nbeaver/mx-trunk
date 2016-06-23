/*
 * Name:    h_file.h
 *
 * Purpose: Supports dictionaries that are initialized from a disk file.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2016 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __H_FILE_H__
#define __H_FILE_H__

typedef struct {
	MX_RECORD *record;

	int dummy;
} MX_FILE_DICTIONARY;

MX_API_PRIVATE mx_status_type mxh_file_create_record_structures(
						MX_RECORD *record );
MX_API_PRIVATE mx_status_type mxh_file_open( MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxh_file_record_function_list;
extern MX_DICTIONARY_FUNCTION_LIST mxh_file_dictionary_function_list;

extern long mxh_file_initial_num_record_fields;

extern MX_RECORD_FIELD_DEFAULTS *mxh_file_initial_rfield_def_ptr;

#endif /* __H_FILE_H__ */

