/*
 * Name:    i_umcbi.c
 *
 * Purpose: MX driver for EG&G Ortec Universal MCB Interface for 32 bits
 *          (Part # A11-B32).
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2003, 2005, 2008, 2010, 2012
 *    Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_UMCBI_DEBUG		FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <windows.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_generic.h"
#include "i_umcbi.h"

/* The following header file is supplied by Ortec as part of the UMCBI kit. */

#include "mcbcio32.h"

MX_RECORD_FUNCTION_LIST mxi_umcbi_record_function_list = {
	NULL,
	mxi_umcbi_create_record_structures,
	mxi_umcbi_finish_record_initialization,
	NULL,
	NULL,
	mxi_umcbi_open,
	mxi_umcbi_close
};

MX_GENERIC_FUNCTION_LIST mxi_umcbi_generic_function_list = {
	mxi_umcbi_getchar,
	mxi_umcbi_putchar,
	mxi_umcbi_read,
	mxi_umcbi_write,
	mxi_umcbi_num_input_bytes_available,
	mxi_umcbi_discard_unread_input,
	mxi_umcbi_discard_unwritten_output
};

MX_RECORD_FIELD_DEFAULTS mxi_umcbi_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS
};

long mxi_umcbi_num_record_fields
		= sizeof( mxi_umcbi_record_field_defaults )
			/ sizeof( mxi_umcbi_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_umcbi_rfield_def_ptr
			= &mxi_umcbi_record_field_defaults[0];

/*==========================*/

MX_EXPORT mx_status_type
mxi_umcbi_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_umcbi_create_record_structures()";

	MX_GENERIC *generic;
	MX_UMCBI *umcbi;

	/* Allocate memory for the necessary structures. */

	generic = (MX_GENERIC *) malloc( sizeof(MX_GENERIC) );

	if ( generic == (MX_GENERIC *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_GENERIC structure." );
	}

	umcbi = (MX_UMCBI *) malloc( sizeof(MX_UMCBI) );

	if ( umcbi == (MX_UMCBI *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Can't allocate memory for MX_UMCBI structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = generic;
	record->record_type_struct = umcbi;
	record->class_specific_function_list = &mxi_umcbi_generic_function_list;

	generic->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_umcbi_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] = "mxi_umcbi_finish_record_initialization()";

	MX_UMCBI *umcbi;

	umcbi = (MX_UMCBI *) record->record_type_struct;

	/* Mark the UMCBI device as being closed. */

	record->record_flags |= ( ~MXF_REC_OPEN );

	/* Null out the array of attached detectors. */

	umcbi->num_detectors = 0;
	umcbi->detector_array = NULL;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_umcbi_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_umcbi_open()";

	MX_UMCBI *umcbi;
	MX_UMCBI_DETECTOR *detector_array;
	BOOL status, outdated;
	INT i, num_detectors;
	DWORD detector_id;
	char name[MIODETNAMEMAX];

	MX_DEBUG( 2, ("mxi_umcbi_open() invoked."));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	umcbi = (MX_UMCBI *) (record->record_type_struct);

	if ( umcbi == (MX_UMCBI *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_UMCBI pointer for record '%s' is NULL.", record->name);
	}

	/* Initialize the connection to the Ortec MCB software. */

	status = MIOStartup();

	if ( status == FALSE ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"MIOStartup() failed." );
	}

	MX_DEBUG( 2,("%s: MIOStartup() succeeded.", fname));

	/* Now get the list of available detectors. */

	status = MIOGetConfigMax( "", &num_detectors );

	if ( status == FALSE ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"MIOGetConfigMax() failed.  Reason = '%s'", 
			mxi_umcbi_strerror() );
	}

	umcbi->num_detectors = num_detectors;

	MX_DEBUG( 2,("%s: num_detectors = %d", fname, (int) num_detectors ));

	if ( num_detectors == 0 ) {
		mx_warning( "No Ortec UMCBI-based devices were detected." );
	}

	detector_array = (MX_UMCBI_DETECTOR *)
			malloc( num_detectors * sizeof(MX_UMCBI_DETECTOR) );

	if ( detector_array == (MX_UMCBI_DETECTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
"Ran out of memory allocating space for %d Ortec MCB detector structures.",
			num_detectors );
	}

	for ( i = 0; i < num_detectors; i++ ) {
		status = MIOGetConfigName( i+1, "", MIODETNAMEMAX, name,
					&detector_id, &outdated );

		if ( status == FALSE ) {
			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"MIOGetConfigName() failed for detector %d.  Reason = '%s'", 
				i, mxi_umcbi_strerror() );
		}
		detector_array[i].detector_number = i+1;
		detector_array[i].detector_id = detector_id;
		detector_array[i].record = NULL;
		strlcpy( detector_array[i].name, name,
				MX_UMCBI_DETECTOR_NAME_LENGTH );

		MX_DEBUG( 2,("%s: Detector %d, detector id = %lu, name = '%s'",
			fname, i+1, detector_array[i].detector_id,
			detector_array[i].name ));
	}

	umcbi->detector_array = detector_array;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_umcbi_close( MX_RECORD *record )
{
	static const char fname[] = "mxi_umcbi_close()";

	MX_UMCBI *umcbi;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	umcbi = (MX_UMCBI *) (record->record_type_struct);

	if ( umcbi == (MX_UMCBI *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_UMCBI pointer for record '%s' is NULL.", record->name);
	}

	/* Shutdown the connection to the universal MCB interface. */

	MIOCleanup();

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_umcbi_getchar( MX_GENERIC *generic, char *c, int flags )
{
	static const char fname[] = "mxi_umcbi_getchar()";

	MX_UMCBI *umcbi;

	if ( generic == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_GENERIC pointer passed is NULL.");
	}

	umcbi = (MX_UMCBI *)(generic->record->record_type_struct);

	if ( umcbi == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_UMCBI pointer for MX_GENERIC structure is NULL." );
	}

	return mx_error(MXE_NOT_YET_IMPLEMENTED, fname,
		"%s is not yet implemented.", fname);
}

MX_EXPORT mx_status_type
mxi_umcbi_putchar( MX_GENERIC *generic, char c, int flags )
{
	static const char fname[] = "mxi_umcbi_putchar()";

	MX_UMCBI *umcbi;

	if ( generic == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_GENERIC pointer passed is NULL.");
	}

	umcbi = (MX_UMCBI *)(generic->record->record_type_struct);

	if ( umcbi == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_UMCBI pointer for MX_GENERIC structure is NULL." );
	}

	return mx_error(MXE_NOT_YET_IMPLEMENTED, fname,
		"%s is not yet implemented.", fname);
}

MX_EXPORT mx_status_type
mxi_umcbi_read( MX_GENERIC *generic, void *buffer, size_t count )
{
	static const char fname[] = "mxi_umcbi_read()";

	MX_UMCBI *umcbi;

	if ( generic == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_GENERIC pointer passed is NULL.");
	}

	umcbi = (MX_UMCBI *)(generic->record->record_type_struct);

	if ( umcbi == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_UMCBI pointer for MX_GENERIC structure is NULL." );
	}

	return mx_error(MXE_NOT_YET_IMPLEMENTED, fname,
		"%s is not yet implemented.", fname);
}

MX_EXPORT mx_status_type
mxi_umcbi_write( MX_GENERIC *generic, void *buffer, size_t count )
{
	static const char fname[] = "mxi_umcbi_write()";

	MX_UMCBI *umcbi;

	if ( generic == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_GENERIC pointer passed is NULL.");
	}

	umcbi = (MX_UMCBI *)(generic->record->record_type_struct);

	if ( umcbi == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_UMCBI pointer for MX_GENERIC structure is NULL." );
	}

	return mx_error(MXE_NOT_YET_IMPLEMENTED, fname,
		"%s is not yet implemented.", fname);
}

MX_EXPORT mx_status_type
mxi_umcbi_num_input_bytes_available( MX_GENERIC *generic,
				unsigned long *input_available )
{
	static const char fname[] = "mxi_umcbi_num_input_bytes_available()";

	MX_UMCBI *umcbi;

	if ( generic == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_GENERIC pointer passed is NULL.");
	}

	umcbi = (MX_UMCBI *)(generic->record->record_type_struct);

	if ( umcbi == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_UMCBI pointer for MX_GENERIC structure is NULL." );
	}

	return mx_error(MXE_NOT_YET_IMPLEMENTED, fname,
		"%s is not yet implemented.", fname);
}

MX_EXPORT mx_status_type
mxi_umcbi_discard_unread_input( MX_GENERIC *generic, int debug_flag )
{
	static const char fname[] = "mxi_umcbi_discard_unread_input()";

	MX_UMCBI *umcbi;

	if ( generic == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_GENERIC pointer passed is NULL.");
	}

	umcbi = (MX_UMCBI *)(generic->record->record_type_struct);

	if ( umcbi == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_UMCBI pointer for MX_GENERIC structure is NULL." );
	}

	return mx_error(MXE_NOT_YET_IMPLEMENTED, fname,
		"%s is not yet implemented.", fname);
}

MX_EXPORT mx_status_type
mxi_umcbi_discard_unwritten_output( MX_GENERIC *generic, int debug_flag )
{
	static const char fname[] = "mxi_umcbi_discard_unwritten_output()";

	MX_UMCBI *umcbi;

	if ( generic == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_GENERIC pointer passed is NULL.");
	}

	umcbi = (MX_UMCBI *)(generic->record->record_type_struct);

	if ( umcbi == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_UMCBI pointer for MX_GENERIC structure is NULL." );
	}

	return mx_error(MXE_NOT_YET_IMPLEMENTED, fname,
		"%s is not yet implemented.", fname);
}

/* === Functions specific to this driver. === */

#if 0
static mx_status_type
mxi_umcbi_wait_for_response_line( MX_UMCBI *umcbi,
			char *buffer,
			int buffer_length,
			int debug_flag )
{
	static const char fname[] = "mxi_umcbi_wait_for_response_line()";

	int i, max_attempts, sleep_ms;
	mx_status_type status;

	i = 0;

	max_attempts = 1;
	sleep_ms = 0;

	for ( i=0; i < max_attempts; i++ ) {
		if ( i > 0 ) {
			MX_DEBUG(-2,
		("%s: No response yet from Ortec UMCBI '%s'.  Retrying...",
				fname, umcbi->record->name ));
		}

		status = mxi_umcbi_getline( umcbi, buffer,
				buffer_length, debug_flag );

		if ( status.code == MXE_SUCCESS ) {
			break;		/* Exit the for() loop. */

		} else if ( status.code != MXE_NOT_READY ) {
			return mx_error( status.code, fname,
			"Error reading response line from Ortec UMCBI." );
		}
		mx_msleep(sleep_ms);
	}

	if ( i >= max_attempts ) {
		status = mxi_umcbi_discard_unread_input(
				umcbi->generic, debug_flag );

		if ( status.code != MXE_SUCCESS ) {
			mx_error( MXE_INTERFACE_IO_ERROR, fname,
"Failed at attempt to discard unread input in buffer for record '%s'",
					umcbi->record->name );
		}

		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"No response seen from Ortec UMCBI." );
	} else {
		return MX_SUCCESSFUL_RESULT;
	}
}
#endif

MX_EXPORT mx_status_type
mxi_umcbi_get_detector_struct( MX_RECORD *umcbi_record,
			int detector_number,
			MX_UMCBI_DETECTOR **detector )
{
	static const char fname[] = "mxi_umcbi_get_detector_struct()";

	MX_UMCBI *umcbi;
	MX_UMCBI_DETECTOR *detector_array;

	if ( umcbi_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"umcbi_record pointer passed was NULL." );
	}
	if ( detector == (MX_UMCBI_DETECTOR **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_UMCBI_DETECTOR pointer passed was NULL." );
	}

	*detector = NULL;

	if ( detector_number <= 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"Illegal detector number %d.  Detector numbers must be positive integers.",
			detector_number );
	}

	umcbi = (MX_UMCBI *) umcbi_record->record_type_struct;

	if ( umcbi == (MX_UMCBI *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"record_type_struct pointer for UMCBI record '%s' is NULL.",
			umcbi_record->name );
	}
	if ( detector_number > umcbi->num_detectors ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Illegal detector number %d.  "
	"The maximum detector number allowed for UMCBI record '%s' is %d.",
			detector_number, umcbi_record->name,
			umcbi->num_detectors );
	}

	if ( umcbi->detector_array == (MX_UMCBI_DETECTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"detector_array pointer for UMCBI record '%s' is NULL.",
			umcbi_record->name );
	}

	*detector = &(umcbi->detector_array)[ detector_number - 1 ];

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_umcbi_command( MX_UMCBI_DETECTOR *detector, char *command,
		char *response, int response_buffer_length,
		int debug_flag )
{
	static const char fname[] = "mxi_umcbi_command()";

	long response_length;
	BOOL mio_status;

	if ( detector == (MX_UMCBI_DETECTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_UMCBI_DETECTOR pointer passed was NULL." );
	}

	if ( command == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"'command' buffer pointer passed was NULL.  No command sent.");
	}

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: command = '%s'", fname, command));
	}

	mio_status = MIOComm( detector->detector_handle,
			command, "", "",
			response_buffer_length, response, &response_length );

	if ( mio_status == FALSE ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Detector '%s' UMCBI communication error for command '%s'.  "
		"Reason = '%s'.", detector->record->name, 
			command, mxi_umcbi_strerror() );
	}

	if ( response != NULL ) {
		if ( response[ response_length - 1 ] == '\r' ) {
			response[ response_length - 1 ] = '\0';
		}
	}

	if ( debug_flag && ( response != NULL ) ) {
		MX_DEBUG(-2,("%s: response = '%s'", fname, response));
	}

	return MX_SUCCESSFUL_RESULT;
}

#if 0

MX_EXPORT mx_status_type
mxi_umcbi_getline( MX_UMCBI_DETECTOR *detector,
		char *buffer, long buffer_length, int debug_flag )
{
	static const char fname[] = "mxi_umcbi_getline()";

	return mx_error(MXE_NOT_YET_IMPLEMENTED, fname,
		"%s is not yet implemented.", fname);
}

#endif

MX_EXPORT mx_status_type
mxi_umcbi_putline( MX_UMCBI_DETECTOR *detector, char *buffer, int debug_flag )
{
	static const char fname[] = "mxi_umcbi_putline()";

	return mxi_umcbi_command( detector, buffer, NULL, 0, debug_flag );
}

MX_EXPORT char *
mxi_umcbi_strerror( void )
{
	static struct {
		int error_code;
		char error_code_text[60];
	} error_code_table[] = {
		{ MIOENONE,	"Function completed successfully" },
		{ MIOEINVALID,	"Invalid function parameter supplied" },
		{ MIOEMCB,	"Detector rejected command" },
		{ MIOEIO,	"MCB communication or network I/O error" },
		{ MIOEMEM,	"Memory allocation error" },
		{ MIOENOTAUTH,	"Detector locked and password does not match" },
		{ MIOENOCONTEXT,"MCBCIO system is not active" }
	};

	static int num_error_codes = sizeof( error_code_table )
					/ sizeof( error_code_table[0] );

	static char buffer[240];

	char *ptr;
	int i, error_code, macro_error_code, micro_error_code;

	/* Return an error string corresponding to the last MCBCIO error
	 * that occurred.
	 */

	error_code = MIOGetLastError( &macro_error_code, &micro_error_code );

	ptr = NULL;

	for ( i = 0; i < num_error_codes; i++ ) {
		if ( error_code == error_code_table[i].error_code ) {
			ptr = error_code_table[i].error_code_text;
			break;
		}
	}

	if ( ptr == NULL ) {
		sprintf( buffer,
	"Unknown error code %d: macro_error_code = %d, micro_error_code = %d",
			error_code, macro_error_code, micro_error_code );
	} else {
		sprintf( buffer,
			"%s: macro_error_code = %d, micro_error_code = %d",
			ptr, macro_error_code, micro_error_code );
	}

	return &buffer[0];
}

