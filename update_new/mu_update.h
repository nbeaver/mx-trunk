/*
 * Name:    mu_update.h
 *
 * Purpose: Header file for routines associated with the MX auto save/restore
 *          and record polling utility.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999, 2001-2002, 2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXUPD_PROTOCOL_ID_NAME_LENGTH	50
#define MXUPD_FIELD_ID_NAME_LENGTH	150

#define MXUPD_UPDATE_ARRAY_BLOCK_SIZE	50

typedef struct {
	char protocol_id[MXUPD_PROTOCOL_ID_NAME_LENGTH+1];
	char record_field_id[MXUPD_FIELD_ID_NAME_LENGTH+1];
	char extra_arguments[MXUPD_FIELD_ID_NAME_LENGTH+1];
	unsigned long update_flags;

	char read_record_name[MXU_RECORD_NAME_LENGTH+1];
	char read_field_name[MXU_FIELD_NAME_LENGTH+1];
	MX_RECORD *read_record;
	MX_RECORD_FIELD *read_value_field;
	void *read_value_pointer;

	char write_record_name[MXU_RECORD_NAME_LENGTH+1];
	char write_field_name[MXU_FIELD_NAME_LENGTH+1];
	MX_RECORD *write_record;
	MX_RECORD_FIELD *write_value_field;
	void *write_value_pointer;

	mx_status_type (*token_constructor) (void *, char *, size_t,
					MX_RECORD *, MX_RECORD_FIELD * );
	mx_status_type (*token_parser) ( void *, char *,
					MX_RECORD *, MX_RECORD_FIELD *,
					MX_RECORD_FIELD_PARSE_STATUS * );

	mx_status_type (*write_function) ( void * );
} MXUPD_UPDATE_LIST_ENTRY;

typedef struct {
	unsigned long num_entries;
	MXUPD_UPDATE_LIST_ENTRY *entry_array;
} MXUPD_UPDATE_LIST;

extern mx_status_type mxupd_create_empty_mx_database(
		MX_RECORD **record_list, int default_display_precision );

extern mx_status_type mxupd_construct_update_list(
		MXUPD_UPDATE_LIST *update_list,
		FILE *update_list_file,
		char *update_list_filename,
		MX_RECORD *record_list );

extern mx_status_type mxupd_poll_update_list( MXUPD_UPDATE_LIST *update_list );

extern mx_status_type mxupd_save_fields_to_autosave_file(
		char *autosave_filename, MXUPD_UPDATE_LIST *update_list );

extern mx_status_type mxupd_restore_fields_from_autosave_files(
		char *autosave1_filename, char *autosave2_filename,
		MXUPD_UPDATE_LIST *update_list );

