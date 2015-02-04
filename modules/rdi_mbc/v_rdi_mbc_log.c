/*
 * Name:    v_rdi_mbc_log.c
 *
 * Purpose: MX operation driver for area detector log files at the
 *          Molecular Biology Consortium beamline.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2013, 2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXV_RDI_MBC_LOG_DEBUG			TRUE

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_cfn.h"
#include "mx_socket.h"
#include "mx_process.h"
#include "mx_image.h"
#include "mx_area_detector.h"
#include "mx_operation.h"

#include "v_rdi_mbc_log.h"

/*---*/

MX_RECORD_FUNCTION_LIST mxv_rdi_mbc_log_record_function_list = {
	NULL,
	mxv_rdi_mbc_log_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxv_rdi_mbc_log_open,
	mxv_rdi_mbc_log_close
};

MX_OPERATION_FUNCTION_LIST mxv_rdi_mbc_log_operation_function_list = {
	mxv_rdi_mbc_log_get_status,
	mxv_rdi_mbc_log_start,
	mxv_rdi_mbc_log_stop
};

/* Record field defaults for 'rdi_mbc_log'. */

MX_RECORD_FIELD_DEFAULTS mxv_rdi_mbc_log_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_OPERATION_STANDARD_FIELDS,
	MXV_RDI_MBC_LOG_STANDARD_FIELDS
};

long mxv_rdi_mbc_log_num_record_fields
	= sizeof( mxv_rdi_mbc_log_record_field_defaults )
	/ sizeof( mxv_rdi_mbc_log_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_rdi_mbc_log_rfield_def_ptr
		= &mxv_rdi_mbc_log_record_field_defaults[0];

/*---*/

static mx_status_type
mxv_rdi_mbc_log_get_pointers( MX_OPERATION *operation,
			MX_RDI_MBC_LOG **rdi_mbc_log,
			MX_AREA_DETECTOR **ad,
			const char *calling_fname )
{
	static const char fname[] =
			"mxv_rdi_mbc_log_get_pointers()";

	MX_RDI_MBC_LOG *rdi_mbc_log_ptr;
	MX_RECORD *ad_record;

	if ( operation == (MX_OPERATION *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_OPERATION pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( operation->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for MX_OPERATION %p is NULL.",
			operation );
	}

	rdi_mbc_log_ptr = (MX_RDI_MBC_LOG *)
				operation->record->record_type_struct;

	if ( rdi_mbc_log_ptr == (MX_RDI_MBC_LOG *) NULL) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	    "The MX_RDI_MBC_LOG pointer for record '%s' is NULL.",
			operation->record->name );
	}

	if ( rdi_mbc_log != (MX_RDI_MBC_LOG **) NULL ) {
		*rdi_mbc_log = rdi_mbc_log_ptr;
	}

	if ( ad != (MX_AREA_DETECTOR **) NULL ) {
		ad_record = rdi_mbc_log_ptr->area_detector_record;

		if ( ad_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The area_detector_record pointer for "
			"operation record '%s' is NULL.",
				operation->record->name );
		}

		*ad = (MX_AREA_DETECTOR *) ad_record->record_class_struct;

		if ( (*ad) == (MX_AREA_DETECTOR *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_AREA_DETECTOR pointer for record '%s' "
			"used by operation '%s' is NULL.",
				ad_record->name,
				operation->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxv_rdi_mbc_log_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxv_rdi_mbc_log_create_record_structures()";

	MX_OPERATION *operation;
	MX_RDI_MBC_LOG *rdi_mbc_log = NULL;

	operation = (MX_OPERATION *) malloc( sizeof(MX_OPERATION) );

	if ( operation == (MX_OPERATION *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_OPERATION structure." );
	}

	rdi_mbc_log = (MX_RDI_MBC_LOG *)
				malloc( sizeof(MX_RDI_MBC_LOG));

	if ( rdi_mbc_log == (MX_RDI_MBC_LOG *) NULL )
	{
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Cannot allocate memory for an MX_RDI_MBC_LOG structure.");
	}

	record->record_superclass_struct = operation;
	record->record_class_struct = NULL;
	record->record_type_struct = rdi_mbc_log;
	record->superclass_specific_function_list = 
			&mxv_rdi_mbc_log_operation_function_list;
	record->class_specific_function_list = NULL;

	operation->record = record;
	rdi_mbc_log->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxv_rdi_mbc_log_open( MX_RECORD *record )
{
	static const char fname[] = "mxv_rdi_mbc_log_open()";

	MX_OPERATION *operation;
	MX_RDI_MBC_LOG *rdi_mbc_log;
	char timestamp[40];
	char log_file_name[ MXU_FILENAME_LENGTH+1 ];
	int saved_errno;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	operation = (MX_OPERATION *) record->record_superclass_struct;

	mx_status = mxv_rdi_mbc_log_get_pointers( operation,
						&rdi_mbc_log, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If the specified filename is not an absolute filename, then
	 * we need to construct the name of the log file using the
	 * control system filename function mx_cfn_construct_filename().
	 */

	mx_status = mx_cfn_construct_filename( MX_CFN_LOGFILE,
					rdi_mbc_log->log_file_name,
					log_file_name,
					sizeof(log_file_name) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Open the log file in append mode so that we do not lose
	 * preexisting log messages.
	 */

	rdi_mbc_log->log_file = fopen( log_file_name, "a" );

	if ( rdi_mbc_log->log_file == (FILE *) NULL ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"The attempt to append to log file '%s' for record '%s' "
		"failed.  Errno = %d, error message = '%s'",
			log_file_name, record->name,
			saved_errno, strerror( saved_errno ) );
	}

	/* Change buffering of the log file to line buffered mode so that
	 * informational messages are written to the log file immediately.
	 */

	setvbuf( rdi_mbc_log->log_file, (char *)NULL, _IOLBF, BUFSIZ );

	/* Write a message to the log file telling the world that we
	 * have opened the file.
	 */

	mx_timestamp( timestamp, sizeof(timestamp) );

	fprintf( rdi_mbc_log->log_file, "%s open\n", timestamp );
	fflush( rdi_mbc_log->log_file );

	return mx_status;
}

MX_EXPORT mx_status_type
mxv_rdi_mbc_log_close( MX_RECORD *record )
{
	static const char fname[] = "mxv_rdi_mbc_log_close()";

	MX_OPERATION *operation;
	MX_RDI_MBC_LOG *rdi_mbc_log;
	char timestamp[40];
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	operation = (MX_OPERATION *) record->record_superclass_struct;

	mx_status = mxv_rdi_mbc_log_get_pointers( operation,
						&rdi_mbc_log, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If the log file is open, write out a message that we are
	 * closing the file and then close it.
	 */

	mx_timestamp( timestamp, sizeof(timestamp) );

	fprintf( rdi_mbc_log->log_file, "%s close\n", timestamp );
	fflush( rdi_mbc_log->log_file );

	fclose( rdi_mbc_log->log_file );

	rdi_mbc_log->log_file = NULL;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxv_rdi_mbc_log_get_status( MX_OPERATION *operation )
{
	static const char fname[] = "mxv_rdi_mbc_log_get_status()";

	MX_RDI_MBC_LOG *rdi_mbc_log = NULL;
	mx_status_type mx_status;

	mx_status = mxv_rdi_mbc_log_get_pointers( operation,
					&rdi_mbc_log, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* FIXME: We probably need a callback to manage getting
	 * and handling the operation status.
	 */

	return mx_status;
}

MX_EXPORT mx_status_type
mxv_rdi_mbc_log_start( MX_OPERATION *operation )
{
	static const char fname[] = "mxv_rdi_mbc_log_start()";

	MX_RDI_MBC_LOG *rdi_mbc_log = NULL;
	MX_AREA_DETECTOR *ad = NULL;
	char timestamp[40];
	char datafile_name_buffer[ MXU_FILENAME_LENGTH+1 ];
	mx_status_type mx_status;

	mx_status = mxv_rdi_mbc_log_get_pointers( operation,
					&rdi_mbc_log, &ad, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_area_detector_construct_next_datafile_name(
					ad->record, FALSE,
					datafile_name_buffer,
					sizeof(datafile_name_buffer) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_area_detector_start( ad->record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_timestamp( timestamp, sizeof(timestamp) );

	fprintf( rdi_mbc_log->log_file, "%s start %s/%s\n",
		timestamp, ad->datafile_directory, datafile_name_buffer );

	fflush( rdi_mbc_log->log_file );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxv_rdi_mbc_log_stop( MX_OPERATION *operation )
{
	static const char fname[] = "mxv_rdi_mbc_log_stop()";

	MX_RDI_MBC_LOG *rdi_mbc_log = NULL;
	MX_AREA_DETECTOR *ad = NULL;
	char timestamp[40];
	mx_status_type mx_status;

	mx_status = mxv_rdi_mbc_log_get_pointers( operation,
					&rdi_mbc_log, &ad, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_area_detector_stop( ad->record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_timestamp( timestamp, sizeof(timestamp) );

	fprintf( rdi_mbc_log->log_file, "%s stop %s/%s\n",
		timestamp, ad->datafile_directory, ad->datafile_pattern );

	fflush( rdi_mbc_log->log_file );

	return MX_SUCCESSFUL_RESULT;
}

