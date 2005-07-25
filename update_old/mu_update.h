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
 * Copyright 1999, 2001-2002 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXUPD_NUM_UPDATE_LISTS		2
#define MXUPD_UPDATE_ARRAY_BLOCK_SIZE	50

typedef struct {
	char record_name[MXU_RECORD_NAME_LENGTH+1];
	char field_name[MXU_FIELD_NAME_LENGTH+1];
	MX_RECORD *local_record;
	MX_RECORD_FIELD *local_value_field;
	void *local_value_pointer;
	mx_status_type (*token_constructor) (void *, char *, size_t,
					MX_RECORD *, MX_RECORD_FIELD * );
	mx_status_type (*token_parser) ( void *, char *,
					MX_RECORD *, MX_RECORD_FIELD *,
					MX_RECORD_FIELD_PARSE_STATUS * );
} MXUPD_UPDATE_LIST_ENTRY;

typedef struct {
	unsigned long num_entries;
	MXUPD_UPDATE_LIST_ENTRY *entry_array;
} MXUPD_UPDATE_LIST;

extern mx_status_type mxupd_connect_to_mx_server( MX_RECORD **server_record,
		char *server_name, int server_port,
		int default_display_precision );

extern mx_status_type mxupd_construct_update_list(
		long num_update_lists,
		MXUPD_UPDATE_LIST *update_list_array,
		FILE *update_list_file,
		char *update_list_filename,
		MX_RECORD *server_record );

extern mx_status_type mxupd_poll_update_list( MXUPD_UPDATE_LIST *update_list );

extern mx_status_type mxupd_save_fields_to_autosave_file(
		char *autosave_filename, MXUPD_UPDATE_LIST *update_list );

extern mx_status_type mxupd_restore_fields_from_autosave_files(
		char *autosave1_filename, char *autosave2_filename,
		MXUPD_UPDATE_LIST *update_list );

