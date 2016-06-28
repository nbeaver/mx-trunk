/*
 * Name:    z_dictionary.h
 *
 * Purpose: Exports MX dictionaries as MX record fields.
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

#ifndef __Z_DICTIONARY_H__
#define __Z_DICTIONARY_H__

#define MXU_DICTIONARY_ARGUMENTS_LENGTH		80

typedef struct {
	MX_RECORD *record;

	MX_DICTIONARY *dictionary;
	char arguments[MXU_DICTIONARY_ARGUMENTS_LENGTH+1];
} MX_DICTIONARY_RECORD;

#define MXZ_DICTIONARY_STANDARD_FIELDS \
  {-1, -1, "arguments", MXFT_STRING, NULL, 1,{MXU_DICTIONARY_ARGUMENTS_LENGTH},\
	MXF_REC_TYPE_STRUCT, offsetof(MX_DICTIONARY_RECORD, arguments), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

MX_API_PRIVATE mx_status_type mxz_dictionary_create_record_structures(
						MX_RECORD *record );
MX_API_PRIVATE mx_status_type mxz_dictionary_open( MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxz_dictionary_record_function_list;

extern long mxz_dictionary_num_record_fields;

extern MX_RECORD_FIELD_DEFAULTS *mxz_dictionary_rfield_def_ptr;

#endif /* __Z_DICTIONARY_H__ */

