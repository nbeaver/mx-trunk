/*
 * Name: mx_camac.c
 *
 * Purpose: MX function library of generic CAMAC operations.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mx_util.h"
#include "mx_types.h"
#include "mx_camac.h"
#include "mx_driver.h"

static mx_status_type
mx_camac_get_pointers( MX_RECORD *camac_record,
			MX_CAMAC **camac,
			MX_CAMAC_FUNCTION_LIST **function_list_ptr,
			const char *calling_fname )
{
	const char fname[] = "mx_camac_get_pointers()";

        if ( camac_record == (MX_RECORD *) NULL ) {
                return mx_error( MXE_NULL_ARGUMENT, fname,
                        "The camac_record pointer passed by '%s' was NULL.",
                        calling_fname );
        }

        if ( camac_record->mx_class != MXI_CAMAC ) {
                return mx_error( MXE_TYPE_MISMATCH, fname,
        "The record '%s' passed by '%s' is not a CAMAC interface.  "
        "(superclass = %ld, class = %ld, type = %ld)",
                        camac_record->name, calling_fname,
                        camac_record->mx_superclass,
                        camac_record->mx_class,
                        camac_record->mx_type );
        }

        if ( camac != (MX_CAMAC **) NULL ) {
                *camac = (MX_CAMAC *) (camac_record->record_class_struct);

                if ( *camac == (MX_CAMAC *) NULL ) {
                        return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
                "The MX_CAMAC pointer for record '%s' passed by '%s' is NULL.",
                                camac_record->name, calling_fname );
                }
        }

        if ( function_list_ptr != (MX_CAMAC_FUNCTION_LIST **) NULL ) {
                *function_list_ptr = (MX_CAMAC_FUNCTION_LIST *)
                                (camac_record->class_specific_function_list);

                if ( *function_list_ptr == (MX_CAMAC_FUNCTION_LIST *) NULL ) {
                        return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_CAMAC_FUNCTION_LIST pointer for record '%s' passed by '%s' is NULL.",
                                camac_record->name, calling_fname );
                }
        }

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_camac_get_lam_status( MX_RECORD *camac_record, int *lam_n )
{
	const char fname[] = "mx_camac_get_lam_status()";

	MX_CAMAC *camac;
	MX_CAMAC_FUNCTION_LIST *fl_ptr;
	mx_status_type ( *fptr ) ( MX_CAMAC *, int * );
	mx_status_type status;

	status = mx_camac_get_pointers( camac_record,
					&camac, &fl_ptr, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	fptr = fl_ptr->get_lam_status;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"get_lam_status function pointer is NULL." );
	}
	
	status = ( *fptr ) ( camac, lam_n );

	return status;
}

MX_EXPORT mx_status_type
mx_camac_controller_command( MX_RECORD *camac_record, int command )
{
	const char fname[] = "mx_camac_controller_command()";

	MX_CAMAC *camac;
	MX_CAMAC_FUNCTION_LIST *fl_ptr;
	mx_status_type ( *fptr ) ( MX_CAMAC *, int );
	mx_status_type status;

	status = mx_camac_get_pointers( camac_record,
					&camac, &fl_ptr, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	fptr = fl_ptr->controller_command;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"controller_command function pointer is NULL." );
	}
	
	status = ( *fptr ) ( camac, command );

	return status;
}

MX_EXPORT mx_status_type
mx_camac( MX_RECORD *camac_record,
	int slot,
	int subaddress,
	int function_code,
	mx_sint32_type *data,
	int *Q,
	int *X )
{
	const char fname[] = "mx_camac()";

	MX_CAMAC *camac;
	MX_CAMAC_FUNCTION_LIST *fl_ptr;
	mx_status_type ( *fptr )( MX_CAMAC *,
		int, int, int, mx_sint32_type *, int *, int * );
	mx_status_type status;

	status = mx_camac_get_pointers( camac_record,
					&camac, &fl_ptr, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	fptr = fl_ptr->camac;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"camac function pointer is NULL." );
	}

	/* Send the actual CAMAC command. */
	
	status = ( *fptr ) ( camac,
			slot, subaddress, function_code, data, Q, X );

	return status;
}

/* mx_camac_qwait() -- A CAMAC command is repeated until Q = 1.
 *                     Needed by E500s.
 */

MX_EXPORT void
mx_camac_qwait( MX_RECORD *camac_record,
	int slot,
	int subaddress,
	int function_code,
	mx_sint32_type *data,
	int *X )
{
	int Q;

	Q = 0;
	*X = 1;

	while( Q == 0 && *X == 1 ) {
		mx_camac( camac_record,
			slot, subaddress, function_code, data, &Q, X );

		mx_msleep(5);
	}
}

