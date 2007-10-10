/*
 * Name:    mx_table.h
 *
 * Purpose: Main header file for the MX table class.
 *
 * Author:  William Lavender
 *
 *----------------------------------------------------------------------------
 *
 * Copyright 2000-2001, 2006-2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_TABLE_H__
#define __MX_TABLE_H__

#include "mx_record.h"

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

/* Table motor types. */

#define MXF_TAB_X	1
#define MXF_TAB_Y	2
#define MXF_TAB_Z	3
#define MXF_TAB_ROLL	4
#define MXF_TAB_PITCH	5
#define MXF_TAB_YAW	6

typedef struct {
	MX_RECORD *record;   /* Pointer to the MX_RECORD structure that points
			      * to this motor.
			      */

	long axis_id;

	mx_bool_type busy;
	mx_bool_type positive_limit_hit;
	mx_bool_type negative_limit_hit;

#if 0
	int position_test_result_flag;
	int generate_position_test_error_message;
#endif

	/* The fields below refer to position values for the currently
	 * selected axis id and _not_ to the table as a whole.
	 */
	
	double destination;
	double position;
	double set_position;
	double test_position;

} MX_TABLE;

typedef struct {
	mx_status_type ( *table_is_busy )( MX_TABLE *table );
	mx_status_type ( *move_absolute )( MX_TABLE *table );
	mx_status_type ( *get_position )( MX_TABLE *table );
	mx_status_type ( *set_position )( MX_TABLE *table );
	mx_status_type ( *soft_abort )( MX_TABLE *table );
	mx_status_type ( *immediate_abort )( MX_TABLE *table );
	mx_status_type ( *positive_limit_hit )( MX_TABLE *table );
	mx_status_type ( *negative_limit_hit )( MX_TABLE *table );
} MX_TABLE_FUNCTION_LIST;

MX_API_PRIVATE mx_status_type mx_table_get_pointers( MX_RECORD *table_record,
		MX_TABLE **table, MX_TABLE_FUNCTION_LIST **function_list_ptr,
		const char *calling_fname );

MX_API mx_status_type mx_table_is_busy( MX_RECORD *table_record,
					long axis_id, mx_bool_type *busy );

MX_API mx_status_type mx_table_soft_abort( MX_RECORD *table_record,
						long axis_id );

MX_API mx_status_type mx_table_immediate_abort( MX_RECORD *table_record,
						long axis_id );

MX_API mx_status_type mx_table_move_absolute( MX_RECORD *table_record,
					long axis_id, double destination );

MX_API mx_status_type mx_table_get_position( MX_RECORD *table_record,
					long axis_id, double *position );

MX_API mx_status_type mx_table_set_position( MX_RECORD *table_record,
					long axis_id, double set_position );

MX_API mx_status_type mx_table_positive_limit_hit( MX_RECORD *table_record,
					long axis_id, mx_bool_type *limit_hit );

MX_API mx_status_type mx_table_negative_limit_hit( MX_RECORD *table_record,
					long axis_id, mx_bool_type *limit_hit );

#ifdef __cplusplus
}
#endif

#endif /* __MX_TABLE_H__ */
