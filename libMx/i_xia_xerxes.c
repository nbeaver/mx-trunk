/*
 * Name:    i_xia_xerxes.c
 *
 * Purpose: MX driver for the X-Ray Instrumentation Associates 
 *          XerXes library used by the DXP-2X multichannel analyzer.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2001-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_XIA_XERXES_DEBUG		FALSE

#define MXI_XIA_XERXES_DEBUG_TIMING	FALSE

#include <stdio.h>
#include <limits.h>

#include "mxconfig.h"

#if HAVE_XIA_XERXES

#if defined(OS_WIN32)
#include <windows.h>
#endif

#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

#include <xia_version.h>
#include <xerxes_errors.h>
#include <xerxes.h>

#include "mx_constants.h"
#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_stdint.h"
#include "mx_array.h"
#include "mx_mca.h"
#include "i_xia_xerxes.h"
#include "d_xia_dxp_mca.h"
#include "u_xia_dxp.h"

#if MXI_XIA_XERXES_DEBUG_TIMING
#  include "mx_hrt_debug.h"
#endif

MX_RECORD_FUNCTION_LIST mxi_xia_xerxes_record_function_list = {
	NULL,
	mxi_xia_xerxes_create_record_structures,
	mxi_xia_xerxes_finish_record_initialization,
	NULL,
	NULL,
	NULL,
	NULL,
	mxi_xia_xerxes_open,
	NULL,
	NULL,
	mxi_xia_xerxes_resynchronize,
	mxi_xia_xerxes_special_processing_setup
};

MX_RECORD_FIELD_DEFAULTS mxi_xia_xerxes_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_XIA_XERXES_STANDARD_FIELDS
};

long mxi_xia_xerxes_num_record_fields
		= sizeof( mxi_xia_xerxes_record_field_defaults )
			/ sizeof( mxi_xia_xerxes_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_xia_xerxes_rfield_def_ptr
			= &mxi_xia_xerxes_record_field_defaults[0];

static mx_status_type mxi_xia_xerxes_set_data_available_flags(
					MX_XIA_XERXES *xia_xerxes,
					int flag_value );

static mx_status_type mxi_xia_xerxes_process_function( void *record_ptr,
						void *record_field_ptr,
						int operation );

static mx_status_type mxi_xia_xerxes_read_dspsymbol_array(
					MX_XIA_DXP_MCA *xia_dxp_mca,
					char *dspsymbol_namestem,
					int num_array_elements,
					unsigned short *dspsymbol_array );

extern const char *mxi_xia_xerxes_strerror( int xia_status );

static mx_status_type
mxi_xia_xerxes_get_pointers( MX_MCA *mca,
			MX_XIA_DXP_MCA **xia_dxp_mca,
			MX_XIA_XERXES **xia_xerxes,
			const char *calling_fname )
{
	static const char fname[] = "mxi_xia_xerxes_get_pointers()";

	MX_RECORD *xia_dxp_record;

	if ( mca == (MX_MCA *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MCA pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( xia_dxp_mca == (MX_XIA_DXP_MCA **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_XIA_DXP_MCA pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( xia_xerxes == (MX_XIA_XERXES **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_XIA_XERXES pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( mca->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_RECORD pointer for the MX_MCA pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*xia_dxp_mca = (MX_XIA_DXP_MCA *) mca->record->record_type_struct;

	xia_dxp_record = (*xia_dxp_mca)->xia_dxp_record;

	if ( xia_dxp_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The xia_dxp_record pointer for MCA record '%s' is NULL.",
			mca->record->name );
	}

	*xia_xerxes = (MX_XIA_XERXES *) xia_dxp_record->record_type_struct;

	if ( (*xia_xerxes) == (MX_XIA_XERXES *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_XIA_XERXES pointer for XIA DXP record '%s' is NULL.",
			xia_dxp_record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* --- */

/* FIXME: The following function probably should not be using tmpnam(). */

static FILE *
mxi_xia_xerxes_open_temporary_file( char *returned_temporary_name,
				size_t maximum_name_length )
{
	static const char fname[] = "mxi_xia_xerxes_open_temporary_file()";

	FILE *temp_file;
	int temp_fd;

#if defined(OS_WIN32)

	char path_buffer[MXU_FILENAME_LENGTH + 1];
	char message_buffer[100];
	long path_length, last_error_code;
	int status;

	if ( maximum_name_length < MXU_FILENAME_LENGTH ) {
		(void) mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The returned temporary name buffer which was %ld "
		"characters long must be at least %ld characters long.",
			maximum_name_length, MXU_FILENAME_LENGTH );

		return NULL;
	}

	/* Find the directory that temporary files are stored in. */

	path_length = GetTempPath( maximum_name_length, path_buffer );

	if ( path_length > maximum_name_length ) {
		(void) mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The temporary file directory name of length %ld "
		"that starts with '%s' "
		"is too long to fit into the buffer of length %ld",
			path_length, path_buffer, maximum_name_length );

		return NULL;
	} else
	if ( path_length == 0 ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		(void) mx_error( MXE_FILE_IO_ERROR, fname,
		"Error getting the Windows temporary file directory.  "
		"Win32 error code = %ld, error message = '%s'",
			last_error_code, message_buffer );

		return NULL;
	}

	/* Now construct a temporary filename in that directory. */

	status = GetTempFileName( path_buffer,
			"XIA", 0, returned_temporary_name );

	if ( status == 0 ) {
		last_error_code = GetLastError();

		mx_win32_error_message( last_error_code,
			message_buffer, sizeof(message_buffer) );

		(void) mx_error( MXE_FILE_IO_ERROR, fname,
		"Error constructing a Windows temporary filename.  "
		"Win32 error code = %ld, error message = '%s'",
			last_error_code, message_buffer );

		return NULL;
	}

#else
	if ( maximum_name_length < L_tmpnam ) {
		errno = ENAMETOOLONG;

		return NULL;
	}

	/* FIXME: There is a potential for a race condition in the following.
	 *        The filename returned by tmpnam() may be in use by another
	 *        file by the time we invoke open().  However, this method
	 *        is somewhat mitigated by the fact that the open() will
	 *        fail if the file already exists due to the inclusion of
	 *        the O_EXCL flag in the call.  This method also has the
	 *        virtue of being fairly portable.
	 */

	if ( tmpnam( returned_temporary_name ) == NULL ) {
		errno = EINVAL;

		return NULL;
	}
#endif

#if defined( OS_WIN32 )
#   if defined( __BORLANDC__ )
	temp_fd = _open( returned_temporary_name,
			_O_TRUNC | _O_EXCL | _O_RDWR );
#   else
	temp_fd = _open( returned_temporary_name,
			_O_TRUNC | _O_EXCL | _O_RDWR,
			_S_IREAD | _S_IWRITE );
#   endif
#elif defined( OS_VXWORKS )
	temp_fd = -1;	/* FIXME: VxWorks doesn't have S_IREAD or S_IWRITE. */
#else
	temp_fd = open( returned_temporary_name,
			O_CREAT | O_TRUNC | O_EXCL | O_RDWR,
			S_IREAD | S_IWRITE );
#endif

	/* If temp_fd is set to -1, an error has occurred during the open.
	 * The open function will already have set errno to an appropriate
	 * value.
	 */

	if ( temp_fd == -1 ) {
		return NULL;
	}

	temp_file = fdopen( temp_fd, "w+" );

	return temp_file;
}

/*==========================*/

MX_EXPORT mx_status_type
mxi_xia_xerxes_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_xia_xerxes_create_record_structures()";

	MX_XIA_XERXES *xia_xerxes;

	/* Allocate memory for the necessary structures. */

	xia_xerxes = (MX_XIA_XERXES *) malloc( sizeof(MX_XIA_XERXES) );

	if ( xia_xerxes == (MX_XIA_XERXES *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Can't allocate memory for MX_XIA_XERXES structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = NULL;;
	record->record_type_struct = xia_xerxes;

	record->class_specific_function_list = NULL;

	xia_xerxes->record = record;

	xia_xerxes->last_measurement_interval = -1.0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_xia_xerxes_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxi_xia_xerxes_finish_record_initialization()";

	MX_XIA_XERXES *xia_xerxes;

	xia_xerxes = (MX_XIA_XERXES *) record->record_type_struct;

	if ( xia_xerxes == (MX_XIA_XERXES *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_XIA_XERXES pointer for record '%s' is NULL.",
			record->name );
	}

	/* We cannot allocate the mca_record_array structures until
	 * mxi_xia_xerxes_open() is invoked, so for now we initialize
	 * these data structures to all zeros.
	 */

	xia_xerxes->num_mcas = 0;
	xia_xerxes->mca_record_array = NULL;

	return MX_SUCCESSFUL_RESULT;
}

/* mxi_xia_xerxes_set_data_available_flags() sets mca->new_data_available
 * and xia_dxp_mca->new_statistics_available for all of the MCAs controlled
 * by this XerXes interface.
 */

static mx_status_type
mxi_xia_xerxes_set_data_available_flags( MX_XIA_XERXES *xia_xerxes,
					int flag_value )
{
	static const char fname[] =
		"mxi_xia_xerxes_set_data_available_flags()";

	MX_RECORD *mca_record, **mca_record_array;
	MX_MCA *mca;
	MX_XIA_DXP_MCA *xia_dxp_mca;
	unsigned long i;

	if ( xia_xerxes == (MX_XIA_XERXES *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_XIA_XERXES pointer passed was NULL." );
	}

	mca_record_array = xia_xerxes->mca_record_array;

	if ( mca_record_array == (MX_RECORD **) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The mca_record_array pointer for 'xia_xerxes' record '%s'.",
			xia_xerxes->record->name );
	}

	for ( i = 0; i < xia_xerxes->num_mcas; i++ ) {

		mca_record = (xia_xerxes->mca_record_array)[i];

		if ( mca_record == (MX_RECORD *) NULL ) {
			/* Skip this MCA slot. */

			continue;
		}

		mca = (MX_MCA *) mca_record->record_class_struct;

		if ( mca == (MX_MCA *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_MCA pointer for MCA record '%s' is NULL.",
				mca_record->name );
		}

		xia_dxp_mca = (MX_XIA_DXP_MCA *) mca_record->record_type_struct;

		if ( xia_dxp_mca == (MX_XIA_DXP_MCA *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		    "The MX_XIA_DXP_MCA pointer for MCA record '%s' is NULL.",
				mca_record->name );
		}

		mca->new_data_available = flag_value;
		xia_dxp_mca->new_statistics_available = flag_value;
	}

	return MX_SUCCESSFUL_RESULT;
}

/* mxi_xia_xerxes_stop_run_and_wait() sends a stop run command to the DXP
 * hardware and then waits for all of the channels to stop running.
 */

#define MXI_XIA_XERXES_DEBUG_STOP_RUN	FALSE

static mx_status_type
mxi_xia_xerxes_stop_run_and_wait( MX_XIA_XERXES *xia_xerxes, int debug_flag )
{
	static const char fname[] = "mxi_xia_xerxes_stop_run_and_wait()";

	unsigned short busy;
	unsigned long i, j, max_attempts, wait_ms;
	int xia_status, at_least_one_channel_is_busy;

#if MXI_XIA_XERXES_DEBUG_TIMING
	MX_HRT_TIMING total_measurement;
#endif

#if ( MXI_XIA_XERXES_DEBUG_TIMING && MXI_XIA_XERXES_DEBUG_STOP_RUN )
	MX_HRT_TIMING measurement;
#endif

	if ( xia_xerxes == (MX_XIA_XERXES *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_XIA_XERXES pointer passed was NULL." );
	}
	if ( xia_xerxes->detector_channel_array == NULL ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"Somehow this function was called before "
		"detector_channel_array was allocated for "
		"XIA interface record '%s'.",
			xia_xerxes->record->name );
	}

	if ( 0 ) {
		MX_DEBUG(-2,("%s: Record '%s'.",
			fname, xia_xerxes->record->name));
	}

#if MXI_XIA_XERXES_DEBUG_TIMING
	MX_HRT_START( total_measurement );
#endif

#if ( MXI_XIA_XERXES_DEBUG_TIMING && MXI_XIA_XERXES_DEBUG_STOP_RUN )
	MX_HRT_START( measurement );
#endif

	xia_status = dxp_stop_run();

#if ( MXI_XIA_XERXES_DEBUG_TIMING && MXI_XIA_XERXES_DEBUG_STOP_RUN )
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname,
	"running dxp_stop_run() for record '%s'", xia_xerxes->record->name );
#endif

	if ( xia_status != DXP_SUCCESS ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Cannot send stop command for XIA DXP interface '%s'.  "
		"Error code = %d, '%s'", xia_xerxes->record->name,
			xia_status, mxi_xia_xerxes_strerror( xia_status ) );
	}

	/* Repeatedly loop through the detector channels waiting for all of 
	 * the BUSY flags to be zero.
	 */

	max_attempts = 50;
	wait_ms = 100;

	for ( i = 0; i < max_attempts; i++ ) {

		at_least_one_channel_is_busy = FALSE;

		for ( j = 0; j < xia_xerxes->num_mcas; j++ ) {

#if ( MXI_XIA_XERXES_DEBUG_TIMING && MXI_XIA_XERXES_DEBUG_STOP_RUN )
			MX_HRT_START( measurement );
#endif
			xia_status = dxp_get_one_dspsymbol(
				&(xia_xerxes->detector_channel_array[j]),
				"BUSY", &busy );

#if ( MXI_XIA_XERXES_DEBUG_TIMING && MXI_XIA_XERXES_DEBUG_STOP_RUN )
			MX_HRT_END( measurement );

			MX_HRT_RESULTS( measurement, fname,
			"for detector_channel_array[%ld] = %d of record '%s'",
				j, xia_xerxes->detector_channel_array[j],
				xia_xerxes->record->name );
#endif

			if ( xia_status != DXP_SUCCESS ) {
				return mx_error(MXE_DEVICE_ACTION_FAILED, fname,
		"Cannot get BUSY parameter value for detector channel %d "
		"belonging to XIA DXP interface '%s'.  Error code = %d, '%s'",
				xia_xerxes->detector_channel_array[j],
				xia_xerxes->record->name,
				xia_status,
				mxi_xia_xerxes_strerror( xia_status ) );
			}

			if ( busy ) {
				at_least_one_channel_is_busy = TRUE;

				break;	/* Exit the for(j...) loop */
			}
		}

		if ( at_least_one_channel_is_busy == FALSE ) {
			break;		/* Exit the for(i...) loop. */
		}

		mx_msleep( wait_ms );
	}

	if ( i >= max_attempts ) {
		return mx_error( MXE_TIMED_OUT, fname,
		"Timed out after waiting over %g seconds for the MCAs "
		"belonging to 'xia_xerxes' record '%s' to stop being busy.",
			0.001 * (double)( max_attempts * wait_ms ),
			xia_xerxes->record->name );
	}

#if MXI_XIA_XERXES_DEBUG_TIMING
	MX_HRT_END( total_measurement );

	MX_HRT_RESULTS( total_measurement, fname,
	"total time for record '%s'", xia_xerxes->record->name );
#endif

	return MX_SUCCESSFUL_RESULT;
}

/* mxi_xia_xerxes_delete_modules_file() deletes
 * the specified temporary XIA modules file.
 */

static mx_status_type
mxi_xia_xerxes_delete_modules_file( char *modules_filename )
{
	static const char fname[] = "mxi_xia_xerxes_delete_modules_file()";

	int fileio_status, saved_errno;

#if defined(OS_WIN32) && defined(_MSC_VER)

	/* FIXME
	 *
	 * dxp_read_config() has a bug where it opens a configuration file
	 * and never closes it.  In addition, it abandons the FILE pointer
	 * so that nobody else can close it either.  This is, of course,
	 * annoying since there is no straightforward way to deal with it.
	 */

#endif
	/* Although the following will work on a Linux/Unix system as is,
	 * it will not work on systems like Microsoft Windows if the file
	 * is currently open.
	 */

	fileio_status = remove( modules_filename );

	if ( fileio_status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname, 
		"The attempt to delete the temporary XIASYSTEMS.CFG "
		"file '%s' failed.  This error is of minor significance "
		"and does not affect the running of the MCA.  "
		"Error code = %d, error status = '%s'.",
			modules_filename, saved_errno, strerror(saved_errno) );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* mxi_xia_xerxes_read_config() either reads the xiasystems.cfg file specified
 * in the MX database or generates a temporary xiasystems.cfg style
 * configuration file.  It then tells Xerxes to load the file.
 */

static mx_status_type
mxi_xia_xerxes_read_config( MX_XIA_XERXES *xia_xerxes )
{
	static const char fname[] = "mxi_xia_xerxes_read_config()";

	FILE *file;
	char modules_filename[MXU_FILENAME_LENGTH+1];
	int xia_status, saved_errno, fileio_status, use_temporary_file;
	mx_status_type mx_status;

	if ( strlen( xia_xerxes->xiasystems_cfg ) <= 0 ) {
		use_temporary_file = TRUE;
	} else {
		use_temporary_file = FALSE;
	}

	if ( use_temporary_file ) {
		/* Open a temporary xiasystems.cfg style file. */

		file = mxi_xia_xerxes_open_temporary_file( modules_filename,
						sizeof(modules_filename) );

		if ( file == NULL ) {
			saved_errno = errno;

			return mx_error( MXE_FILE_IO_ERROR, fname,
		"Unable to open temporary XIASYSTEMS.CFG file '%s'.  "
		"Error code = %d, error status = '%s'.",
			modules_filename, saved_errno, strerror(saved_errno) );
		}
	} else {
		/* Open the specified xiasystems.cfg file. */

		strlcpy( modules_filename, xia_xerxes->xiasystems_cfg,
				MXU_FILENAME_LENGTH );

		file = fopen( modules_filename, "r" );

		if ( file == NULL ) {
			saved_errno = errno;

			return mx_error( MXE_FILE_IO_ERROR, fname,
		"Unable to open XIASYSTEMS.CFG file '%s'.  "
		"Error code = %d, error status = '%s'.",
			modules_filename, saved_errno, strerror(saved_errno) );
		}
	}

	MX_DEBUG( 2,("%s: Temporary Xerxes module config file = '%s'.",
		fname, modules_filename));

	/* Write out the contents of the file. */

	fprintf( file, "preamp    NULL\n" );
	fprintf( file, "modules   NULL\n" );
	fprintf( file, "%-9s NULL\n", xia_xerxes->module_type );

	fileio_status = fclose( file );

	if ( fileio_status != 0 ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"An error occurred while closing temporary "
		"XerXes configuration file '%s'.  "
		"Error code = %d, error status = '%s'.",
			modules_filename, saved_errno, strerror(saved_errno) );
	}

	/* Tell XerXes to read the temporary config file. */

	xia_status = dxp_read_config( modules_filename );

	if ( xia_status != DXP_SUCCESS ) {
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"Unable to read the modules configuration from "
		"the XerXes configuration file '%s'.  "
		"Error code = %d, '%s'", modules_filename,
			xia_status, mxi_xia_xerxes_strerror( xia_status ) );
	}

	if ( use_temporary_file ) {
	    mx_status = mxi_xia_xerxes_delete_modules_file(modules_filename);
	}

	return MX_SUCCESSFUL_RESULT;
}

/*
 * mxi_xia_xerxes_restore_config() encapsulates all of the operations that
 * must happen when dxp_restore_config() is called.
 */

static mx_status_type
mxi_xia_xerxes_restore_config( MX_XIA_XERXES *xia_xerxes )
{
	static const char fname[] = "mxi_xia_xerxes_restore_config()";

	MX_RECORD *mca_record;
	MX_MCA *mca;
	MX_XIA_DXP_MCA *xia_dxp_mca;
	MX_LIST_HEAD *list_head;
	int i, xia_status, num_mcas, num_dxp_chan;
	int config_file_lun, config_file_mode;
	int max_attempts, timed_out;
	unsigned long wait_ms;
	unsigned short gate, resume_flag;
	mx_status_type mx_status;

#if MXI_XIA_XERXES_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	/* Open the detector configuration file. */

	mx_info("Loading Xerxes configuration file '%s'.",
		xia_xerxes->config_filename);

	max_attempts = 10;
	wait_ms = 100;

	for ( i = 0; i < max_attempts; i++ ) {

		timed_out = FALSE;

		if ( i > 0 ) {
			mx_warning(
		"*** The previous load attempt timed out.  Retry %d ***", i);
		}

		config_file_mode = 2;

#if MXI_XIA_XERXES_DEBUG_TIMING
		MX_HRT_START( measurement );
#endif

		xia_status = dxp_open_file( &config_file_lun,
					xia_xerxes->config_filename,
					&config_file_mode );

#if MXI_XIA_XERXES_DEBUG_TIMING
		MX_HRT_END( measurement );

		MX_HRT_RESULTS( measurement, fname,
		  "dxp_open_file() for '%s'", xia_xerxes->config_filename );
#endif

		if ( xia_status != DXP_SUCCESS ) {
			return mx_error( MXE_FILE_IO_ERROR, fname,
		"Unable to open the Xerxes detector configuration file '%s'.  "
		"Error code = %d, '%s'",
			xia_xerxes->config_filename,
			xia_status, mxi_xia_xerxes_strerror( xia_status ) );
		}

		/* Restore the detector configuration. */

#if MXI_XIA_XERXES_DEBUG_TIMING
		MX_HRT_START( measurement );
#endif

		xia_status = dxp_restore_config( &config_file_lun );

#if MXI_XIA_XERXES_DEBUG_TIMING
		MX_HRT_END( measurement );

		MX_HRT_RESULTS( measurement, fname, "dxp_restore_config()" );
#endif

		switch( xia_status ) {
		case DXP_SUCCESS:
			timed_out = FALSE;
			break;
		case DXP_DSPTIMEOUT:
			timed_out = TRUE;
			break;
		default:
			return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"Unable to restore the detector configuration from "
		"the Xerxes configuration file '%s'.  "
		"Error code = %d, '%s'",
			xia_xerxes->config_filename,
			xia_status, mxi_xia_xerxes_strerror( xia_status ) );
		}

		/* Close the detector configuration file. */

#if MXI_XIA_XERXES_DEBUG_TIMING
		MX_HRT_START( measurement );
#endif

		xia_status = dxp_close_file( &config_file_lun );

#if MXI_XIA_XERXES_DEBUG_TIMING
		MX_HRT_END( measurement );

		MX_HRT_RESULTS( measurement, fname, "dxp_close_file()" );
#endif

		if ( xia_status != DXP_SUCCESS ) {
			return mx_error( MXE_FILE_IO_ERROR, fname,
		"Unable to close the Xerxes detector configuration file '%s'.  "
		"Error code = %d, '%s'",
			xia_xerxes->config_filename,
			xia_status, mxi_xia_xerxes_strerror( xia_status ) );
		}

		if ( timed_out == FALSE ) {
			break;		/* Exit the for() loop. */
		}

		mx_msleep( wait_ms );
	}

	if ( i >= max_attempts ) {
		mx_status = mx_error( MXE_TIMED_OUT, fname,
		"The attempt to load Xerxes configuration file '%s' timed out "
		"after %d failed attempts",
			xia_xerxes->config_filename, max_attempts);

		/* We handle this differently depending on whether the 
		 * current process is an MX server or not.
		 */

		list_head = mx_get_record_list_head_struct(xia_xerxes->record);

		if ( list_head->is_server ) {
			mx_info( "The Xerxes timeout is a fatal error, "
				"so the MX server is aborting the startup." );
			mx_info( "We suggest that you try restarting the "
				"MX server since most of the time this will "
				"fix the problem." );
			exit(1);
		} else {
			return mx_status;
		}
	}

	/* Find out how many detector channels (MCAs) there are. */

	xia_status = dxp_ndxpchan( &num_mcas );

	if ( xia_status != DXP_SUCCESS ) {
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"Unable to find out how many detector channels are in "
		"the current configuration '%s'.  "
		"Error code = %d, '%s'",
			xia_xerxes->config_filename,
			xia_status, mxi_xia_xerxes_strerror( xia_status ) );
	}

	/* If detector_channel_array is not NULL, then this is not the first
	 * time that mxi_xia_xerxes_restore_config() has been invoked.
	 * We need to see if the number of MCAs has changed since changing
	 * the number of MCAs is not supported by MX.
	 */

	if ( xia_xerxes->detector_channel_array != NULL ) {
		if ( num_mcas != xia_xerxes->num_mcas ) {
			return mx_error( MXE_UNSUPPORTED, fname,
		"The detector configuration file '%s' has a different number "
		"of MCAs (%d) than the previous configuration did (%ld).  "
		"MX does not support changing the number of MCAs after the "
		"program has started, so the only way to recover is to "
		"shut down MX and start it again.",
				xia_xerxes->config_filename,
				num_mcas, xia_xerxes->num_mcas );
		}
	} else {
		/* detector_channel_array is NULL, so we must create it. */

		xia_xerxes->num_mcas = num_mcas;

		xia_xerxes->detector_channel_array =
			(int *) malloc( num_mcas * sizeof(int) );

		if ( xia_xerxes->detector_channel_array == NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory allocating a %d element array "
			"of detector_channel values.", num_mcas );
		}

		/* Fill in the detector channel array.
		 *
		 * Note: 'num_dxp_chan' seems redundant since we must call
		 * dxp_ndxpchan() ahead of time in order to know how big to
		 * make detector_channel_array anyway.  But, what do I know...
		 */

		xia_status = dxp_get_detectors( &num_dxp_chan, 
					xia_xerxes->detector_channel_array );

		if ( xia_status != DXP_SUCCESS ) {
			return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"Unable to get the list of detector channels for "
		"the current configuration '%s'.  "
		"Error code = %d, '%s'",
			xia_xerxes->config_filename,
			xia_status, mxi_xia_xerxes_strerror( xia_status ) );
		}

		/* Allocate and initialize an array to store pointers to all of
		 * the MCA records used by this interface.
		 */

		xia_xerxes->mca_record_array = ( MX_RECORD ** ) malloc(
				xia_xerxes->num_mcas * sizeof( MX_RECORD * ) );

		if ( xia_xerxes->mca_record_array == ( MX_RECORD ** ) NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %ld element array "
		"of MX_RECORD pointers for the mca_record_array data "
		"structure of record '%s'.",
				xia_xerxes->num_mcas, xia_xerxes->record->name);
		}

		for ( i = 0; i < xia_xerxes->num_mcas; i++ ) {
			xia_xerxes->mca_record_array[i] = NULL;
		}
	}

	/*
	 * Patrick Franz of XIA recommends that a short DXP run be executed
	 * after a restore configuration is done.
	 */

	gate = 1;
	resume_flag = 0;

#if MXI_XIA_XERXES_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	xia_status = dxp_start_run( &gate, &resume_flag );

#if MXI_XIA_XERXES_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname, "dxp_start_run()" );
#endif

	if ( xia_status != DXP_SUCCESS ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Unable to start a run for XIA DXP interface '%s'.  "
		"Error code = %d, '%s'", xia_xerxes->record->name,
			xia_status, mxi_xia_xerxes_strerror( xia_status ) );
	}

	/* Wait for 1 millisecond. */

	mx_sleep(1);

	/* Send a stop run command to make sure that all DXP channels
	 * are stopped.
	 */

	mx_status = mxi_xia_xerxes_stop_run_and_wait( xia_xerxes,
						MXI_XIA_XERXES_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Invalidate the old presets for all of the MCAs by setting the
	 * old preset type to an illegal value.
	 */

	if ( xia_xerxes->mca_record_array != NULL ) {
		for ( i = 0; i < xia_xerxes->num_mcas; i++ ) {
			mca_record = xia_xerxes->mca_record_array[i];

			if ( mca_record != NULL ) {
				mca = (MX_MCA *)
					mca_record->record_class_struct;

				xia_dxp_mca = (MX_XIA_DXP_MCA *)
					mca_record->record_type_struct;

				xia_dxp_mca->old_preset_type
					= (unsigned long) MX_ULONG_MAX;

				mca->new_data_available = TRUE;
				xia_dxp_mca->new_statistics_available = TRUE;
			}
		}
	}


	mx_info("Successfully loaded Xerxes configuration file '%s'.",
			xia_xerxes->config_filename);

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_xia_xerxes_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_xia_xerxes_open()";

	MX_XIA_XERXES *xia_xerxes;
	char version_string[80];
	int xia_status;
	mx_status_type mx_status;

#if MXI_XIA_XERXES_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	MX_DEBUG( 2,("%s invoked.", fname));

#if MXI_XIA_XERXES_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	xia_xerxes = (MX_XIA_XERXES *) (record->record_type_struct);

	if ( xia_xerxes == (MX_XIA_XERXES *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_XIA_XERXES pointer for record '%s' is NULL.", record->name);
	}

	/* Redirect Xerxes output to the MX output functions. */

	mx_status = mxu_xia_dxp_replace_output_functions();

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Initialize mca_record_array to NULL, so that restore_config
	 * below knows not to try to change the MCA presets.
	 */

	xia_xerxes->mca_record_array = NULL;

	/* Initialize detector_channel_array to NULL, so that restore_config
	 * below knows that it needs to create the array.
	 */

	xia_xerxes->detector_channel_array = NULL;

	/* Initialize Xerxes. */

	xia_status = dxp_init_library();

	if ( xia_status != DXP_SUCCESS ) {
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"Unable to initialize XerXes.  "
		"Error code = %d, '%s'",
			xia_status, mxi_xia_xerxes_strerror( xia_status ) );
	}

	/* Display the version of Xerxes/Handel. */

	dxp_get_version_info( NULL, NULL, NULL, version_string );

	mx_info("MX is using Xerxes %s", version_string );

	/* Read the modules configuration. */

	mx_status = mxi_xia_xerxes_read_config( xia_xerxes );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Restore the detector configuration. */

	mx_status = mxi_xia_xerxes_restore_config( xia_xerxes );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_XIA_XERXES_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname, record->name );
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_xia_xerxes_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxi_xia_xerxes_resynchronize()";

	MX_XIA_XERXES *xia_xerxes;
	mx_status_type mx_status;

#if MXI_XIA_XERXES_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	xia_xerxes = (MX_XIA_XERXES *) (record->record_type_struct);

	if ( xia_xerxes == (MX_XIA_XERXES *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_XIA_XERXES pointer for record '%s' is NULL.", record->name);
	}

#if MXI_XIA_XERXES_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	mx_status = mxi_xia_xerxes_restore_config( xia_xerxes );

#if MXI_XIA_XERXES_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname, record->name );
#endif

	return mx_status;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_xia_xerxes_is_busy( MX_MCA *mca,
			mx_bool_type *busy_flag,
			int debug_flag )
{
	static const char fname[] = "mxi_xia_xerxes_is_busy()";

	MX_XIA_DXP_MCA *xia_dxp_mca;
	MX_XIA_XERXES *xia_xerxes;
	int xia_status, active;
	mx_status_type mx_status;

#if MXI_XIA_XERXES_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	mx_status = mxi_xia_xerxes_get_pointers( mca,
					&xia_dxp_mca, &xia_xerxes, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_XIA_XERXES_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	xia_status = dxp_isrunning( &(xia_dxp_mca->detector_channel), &active );

#if MXI_XIA_XERXES_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname, mca->record->name );
#endif

	if ( xia_status != DXP_SUCCESS ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Cannot get DXP running status for MCA '%s'.  "
		"Error code = %d, '%s'",
			mca->record->name, xia_status,
			mxi_xia_xerxes_strerror( xia_status ) );
	}

	/* We are only interested in the two lowest order bits.
	 *
	 * Bit 0 tells you whether or not the board things a run
	 * is active.  Bit 1 tells you whether or not XerXes
	 * thinks a run is active.
	 */

	active &= 0x3;

	MX_DEBUG( 2,("%s: active = %#x", fname, active));

	switch( active ) {
	case 0x0:
	case 0x2:
		*busy_flag = FALSE;
		break;
	case 0x1:
		(void) mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"Inconsistent active state flags %#x: "
		"The DXP board for MCA '%s' says that it is active, "
		"but XerXes says that it is not.",
			active, mca->record->name );
		*busy_flag = TRUE;
		break;
	case 0x3:
		*busy_flag = TRUE;
		break;
	default:
		return mx_error( MXE_FUNCTION_FAILED, fname,
		"It should not be possible to get to this message.  "
		"If you do, then something peculiar has happened "
		"for MCA '%s' and this should be reported to the programmer.",
			mca->record->name );
		break;
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_xia_xerxes_read_parameter( MX_MCA *mca,
			char *parameter_name,
			unsigned long *value_ptr,
			int debug_flag )
{
	static const char fname[] = "mxi_xia_xerxes_read_parameter()";

	MX_XIA_DXP_MCA *xia_dxp_mca;
	MX_XIA_XERXES *xia_xerxes;
	unsigned short short_value;
	int xia_status;
	mx_status_type mx_status;

#if MXI_XIA_XERXES_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	mx_status = mxi_xia_xerxes_get_pointers( mca,
					&xia_dxp_mca, &xia_xerxes, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( debug_flag ) {
		MX_DEBUG(-2,
		("%s: Record '%s', channel %d, parameter '%s'.",
			fname, mca->record->name,
			xia_dxp_mca->detector_channel, parameter_name));
	}

#if MXI_XIA_XERXES_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	xia_status = dxp_get_one_dspsymbol( &(xia_dxp_mca->detector_channel),
						parameter_name, &short_value );

#if MXI_XIA_XERXES_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname,
		"for record '%s', channel %d, parameter '%s'",
		mca->record->name,
		xia_dxp_mca->detector_channel,
		parameter_name );
#endif

	if ( xia_status != DXP_SUCCESS ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Cannot read DXP parameter '%s' for detector channel %d "
		"of XIA DXP interface '%s'.  "
		"Error code = %d, '%s'",
			parameter_name, xia_dxp_mca->detector_channel,
			mca->record->name, xia_status,
			mxi_xia_xerxes_strerror( xia_status ) );
	}

	*value_ptr = (unsigned long) short_value;

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: value = %lu", fname, *value_ptr));
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_xia_xerxes_write_parameter( MX_MCA *mca,
			char *parameter_name,
			unsigned long value,
			int debug_flag )
{
	static const char fname[] = "mxi_xia_xerxes_write_parameter()";

	MX_XIA_DXP_MCA *xia_dxp_mca;
	MX_XIA_XERXES *xia_xerxes;
	unsigned short short_value;
	int xia_status;
	mx_status_type mx_status;

#if MXI_XIA_XERXES_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	mx_status = mxi_xia_xerxes_get_pointers( mca,
					&xia_dxp_mca, &xia_xerxes, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( debug_flag ) {
		MX_DEBUG(-2,
		("%s: Record '%s', channel %d, parameter '%s', value = %lu.",
			fname, mca->record->name, xia_dxp_mca->detector_channel,
			parameter_name, (unsigned long) value));
	}

	/* Make sure the run is stopped before trying to change a parameter. */

	mx_status = mxi_xia_xerxes_stop_run_and_wait( xia_xerxes,
						MXI_XIA_XERXES_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Send the parameter value. */

	short_value = (unsigned short) value;

#if MXI_XIA_XERXES_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	xia_status = dxp_set_one_dspsymbol( &(xia_dxp_mca->detector_channel),
						parameter_name, &short_value );

#if MXI_XIA_XERXES_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname,
		"for record '%s', channel %d, parameter '%s'",
		mca->record->name,
		xia_dxp_mca->detector_channel,
		parameter_name );
#endif

	if ( xia_status != DXP_SUCCESS ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Cannot write the value %hu to DXP parameter '%s' "
		"for detector channel %d of XIA DXP interface '%s'.  "
		"Error code = %d, '%s'", short_value,
			parameter_name, xia_dxp_mca->detector_channel,
			mca->record->name, xia_status,
			mxi_xia_xerxes_strerror( xia_status ) );
	}

	if ( debug_flag ) {
		MX_DEBUG(-2,
		("%s: Write to parameter '%s' succeeded.",
			fname, parameter_name));
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_xia_xerxes_write_parameter_to_all_channels( MX_MCA *mca,
			char *parameter_name,
			unsigned long value,
			int debug_flag )
{
	static const char fname[] =
		"mxi_xia_xerxes_write_parameter_to_all_channels()";

	MX_XIA_DXP_MCA *xia_dxp_mca;
	MX_XIA_XERXES *xia_xerxes;
	unsigned short short_value;
	int xia_status;
	mx_status_type mx_status;

#if MXI_XIA_XERXES_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	mx_status = mxi_xia_xerxes_get_pointers( mca,
					&xia_dxp_mca, &xia_xerxes, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: Record '%s', parameter '%s', value = %lu.",
			fname, mca->record->name,
			parameter_name, (unsigned long) value));
	}

	/* Make sure the run is stopped before trying to change a parameter. */

	mx_status = mxi_xia_xerxes_stop_run_and_wait( xia_xerxes,
						MXI_XIA_XERXES_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	short_value = (unsigned short) value;

#if 0
	/* The canonical method which does not work. */

	xia_status = dxp_set_dspsymbol( parameter_name, &short_value );

	if ( xia_status != DXP_SUCCESS ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Cannot write the value %hu to DXP parameter '%s' "
		"for all channels of XIA DXP interface '%s'.  ",
		"Error code = %d, '%s'",
			short_value, parameter_name, mca->record->name,
			xia_status, mxi_xia_xerxes_strerror( xia_status ) );
	}
#else
	/* A temporary kludge which does work. */

	{
		MX_RECORD *mca_record;
		MX_XIA_DXP_MCA *local_dxp_mca;
		unsigned long i;

		for ( i = 0; i < xia_xerxes->num_mcas; i++ ) {

			mca_record = xia_xerxes->mca_record_array[i];

			if ( mca_record == (MX_RECORD *) NULL ) {

				continue;    /* Skip this one. */
			}

			local_dxp_mca = (MX_XIA_DXP_MCA *)
					mca_record->record_type_struct;

			MX_DEBUG( 2,("%s: writing to detector channel %d.",
				fname, local_dxp_mca->detector_channel));

#if MXI_XIA_XERXES_DEBUG_TIMING
			MX_HRT_START( measurement );
#endif

			xia_status = dxp_set_one_dspsymbol(
					&(local_dxp_mca->detector_channel),
					parameter_name, &short_value );

#if MXI_XIA_XERXES_DEBUG_TIMING
			MX_HRT_END( measurement );

			MX_HRT_RESULTS( measurement, fname,
				"for record '%s', channel %d, parameter '%s'",
				mca->record->name,
				local_dxp_mca->detector_channel,
				parameter_name );
#endif

			if ( xia_status != DXP_SUCCESS ) {
				return mx_error( MXE_DEVICE_ACTION_FAILED,fname,
			"Cannot write the value %hu to DXP parameter '%s' "
			"for detector channel %d of XIA DXP interface '%s'.  "
			"Error code = %d, '%s'", short_value,
				parameter_name, local_dxp_mca->detector_channel,
				mca->record->name, xia_status,
				mxi_xia_xerxes_strerror( xia_status ) );
			}
		}
	}
#endif
	if ( debug_flag ) {
		MX_DEBUG(-2,
		("%s: Write parameter '%s' to all channels succeeded.",
			fname, parameter_name));
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_xia_xerxes_start_run( MX_MCA *mca,
			mx_bool_type clear_flag,
			int debug_flag )
{
	static const char fname[] = "mxi_xia_xerxes_start_run()";

	MX_XIA_DXP_MCA *xia_dxp_mca;
	MX_XIA_XERXES *xia_xerxes;
	unsigned long i, wait_ms, max_retries;
	unsigned short gate, resume_flag;
	int xia_status;
	mx_status_type mx_status;

#if MXI_XIA_XERXES_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	mx_status = mxi_xia_xerxes_get_pointers( mca,
					&xia_dxp_mca, &xia_xerxes, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: Record '%s', clear_flag = %d.",
				fname, mca->record->name, clear_flag));
	}

	mxi_xia_xerxes_set_data_available_flags( xia_xerxes, TRUE );

	gate = 1;

	if ( clear_flag ) {
		resume_flag = 0;
	} else {
		resume_flag = 1;
	}

#if MXI_XIA_XERXES_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	xia_status = dxp_start_run( &gate, &resume_flag );

#if MXI_XIA_XERXES_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname,
		"dxp_start_run() for record '%s'",
		mca->record->name );
#endif

	if ( xia_status == DXP_SUCCESS ) {
		/* dxp_start_run() succeeded, so we are done for now. */

		return MX_SUCCESSFUL_RESULT;
	}

	if ( xia_status != DXP_RUNACTIVE ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Cannot start data acquisition for XIA DXP interface '%s'.  "
		"Error code = %d, '%s'", mca->record->name,
			xia_status, mxi_xia_xerxes_strerror( xia_status ) );
	}

	/* If we get here, the status returned by dxp_start_run()
	 * was DXP_RUNACTIVE.  If this happens, we attempt to stop
	 * the run and then execute dxp_start_run() again.
	 */

	wait_ms = 100;
	max_retries = 5;

	for ( i = 0; i < max_retries; i++ ) {

		MX_DEBUG(-2,
		("%s: The start run attempt failed for MCA '%s' since "
		"the MCA said a run was active.  We will attempt to "
		"stop the run and retry starting the run.  Retry # %lu",
		fname, mca->record->name, i+1 ));

		mx_status = mxi_xia_xerxes_stop_run_and_wait( xia_xerxes, 0 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		xia_status = dxp_start_run( &gate, &resume_flag );

		if ( xia_status == DXP_SUCCESS ) {
			/* dxp_start_run() succeeded, so we are done for now. */

			return MX_SUCCESSFUL_RESULT;
		}

		if ( xia_status == DXP_RUNACTIVE ) {
			/* Go back to the top of the loop and try again. */

			continue;
		}

		/* We received some error code other than DXP_SUCCESS
		 * or DXP_RUNACTIVE.
		 */

		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Cannot start data acquisition for XIA DXP interface '%s'.  "
		"Error code = %d, '%s'", mca->record->name,
			xia_status, mxi_xia_xerxes_strerror( xia_status ) );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_xia_xerxes_stop_run( MX_MCA *mca,
			int debug_flag )
{
	static const char fname[] = "mxi_xia_xerxes_stop_run()";

	MX_XIA_DXP_MCA *xia_dxp_mca;
	MX_XIA_XERXES *xia_xerxes;
	mx_status_type mx_status;

	mx_status = mxi_xia_xerxes_get_pointers( mca,
					&xia_dxp_mca, &xia_xerxes, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_xia_xerxes_stop_run_and_wait( xia_xerxes, debug_flag );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_xia_xerxes_read_spectrum( MX_MCA *mca,
			int debug_flag )
{
	static const char fname[] = "mxi_xia_xerxes_read_spectrum()";

	MX_XIA_DXP_MCA *xia_dxp_mca;
	MX_XIA_XERXES *xia_xerxes;
	int xia_status;
	unsigned long *array_ptr;
	mx_status_type mx_status;

#if MXI_XIA_XERXES_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	mx_status = mxi_xia_xerxes_get_pointers( mca,
					&xia_dxp_mca, &xia_xerxes, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( xia_dxp_mca->use_mca_channel_array ) {
		array_ptr = mca->channel_array;
	} else {
		array_ptr = xia_dxp_mca->spectrum_array;
	}

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: reading out %ld channels from MCA '%s'.",
			fname, mca->current_num_channels, mca->record->name));
	}

#if MXI_XIA_XERXES_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	xia_status = dxp_readout_detector_run(
				&(xia_dxp_mca->detector_channel),
				NULL, NULL,
				array_ptr );

#if MXI_XIA_XERXES_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname,
		"for record '%s'", mca->record->name );
#endif

	if ( xia_status != DXP_SUCCESS ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Cannot read the spectrum for XIA MCA '%s'.  "
		"Error code = %d, '%s'", mca->record->name,
			xia_status, mxi_xia_xerxes_strerror( xia_status ) );
	}

#if 1
	/* FIXME - For some reason the value in the last bin is always bad.
	 * Thus, we set it to zero.  (W. Lavender)
	 */

	array_ptr[ mca->current_num_channels - 1 ] = 0;
#endif

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: readout from MCA '%s' complete.",
			fname, mca->record->name));
	}

	return MX_SUCCESSFUL_RESULT;
}

#define TWO_TO_THE_32ND_POWER	4294967296.0

MX_EXPORT mx_status_type
mxi_xia_xerxes_read_statistics( MX_MCA *mca,
			int debug_flag )
{
	static const char fname[] = "mxi_xia_xerxes_read_statistics()";

	MX_XIA_DXP_MCA *xia_dxp_mca;
	MX_XIA_XERXES *xia_xerxes;
	unsigned long nevent;
	int xia_status;
	unsigned short parameter_array[3];
	double real_time;
	mx_status_type mx_status;

#if MXI_XIA_XERXES_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	mx_status = mxi_xia_xerxes_get_pointers( mca,
					&xia_dxp_mca, &xia_xerxes, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_XIA_XERXES_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

	xia_status = dxp_get_statistics( &(xia_dxp_mca->detector_channel),
					&(mca->live_time),
					&(xia_dxp_mca->input_count_rate),
					&(xia_dxp_mca->output_count_rate),
					&nevent );

#if MXI_XIA_XERXES_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname,
		"dxp_get_statistics() for record '%s'",
		mca->record->name );
#endif

	if ( xia_status != DXP_SUCCESS ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Cannot read statistics for XIA DXP interface '%s'.  "
		"Error code = %d, '%s'", mca->record->name,
			xia_status, mxi_xia_xerxes_strerror( xia_status ) );
	}

	if ( debug_flag ) {
		MX_DEBUG(-2,(
"%s: Record '%s', channel %d, live_time = %g, icr = %g, ocr = %g, nevent = %lu",
			fname, mca->record->name, xia_dxp_mca->detector_channel,
			mca->live_time, xia_dxp_mca->input_count_rate,
			xia_dxp_mca->output_count_rate, nevent ));
	}

	xia_dxp_mca->num_events = (unsigned long) nevent;

	xia_dxp_mca->num_fast_peaks = (unsigned long) mx_round(
				xia_dxp_mca->input_count_rate * mca->live_time);

	xia_dxp_mca->num_underflows = 0;
	xia_dxp_mca->num_overflows = 0;

	mx_status = mxi_xia_xerxes_read_dspsymbol_array( xia_dxp_mca,
					"REALTIME", 3, parameter_array );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	real_time = (double) parameter_array[1] + 65536.0 * parameter_array[0]
				+ TWO_TO_THE_32ND_POWER * parameter_array[2];

	real_time *= xia_dxp_mca->runtime_clock_tick;

	mca->real_time = real_time;

#if 1
	xia_dxp_mca->num_underflows = 0;
	xia_dxp_mca->num_overflows = 0;
#else
	mx_status = mxi_xia_xerxes_read_dspsymbol_array( xia_dxp_mca,
					"UNDERFLOWS", 2, parameter_array );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	xia_dxp_mca->num_underflows = (unsigned long) parameter_array[0]
				+ 65536L * (unsigned long) parameter_array[1];

	mx_status = mxi_xia_xerxes_read_dspsymbol_array( xia_dxp_mca,
					"OVERFLOWS", 2, parameter_array );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	xia_dxp_mca->num_overflows = (unsigned long) parameter_array[0]
				+ 65536L * (unsigned long) parameter_array[1];
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_xia_xerxes_get_baseline_array( MX_MCA *mca,
			int debug_flag )
{
	static const char fname[] = "mxi_xia_xerxes_get_baseline_array()";

	MX_XIA_DXP_MCA *xia_dxp_mca;
	MX_XIA_XERXES *xia_xerxes;
	int xia_status;
	unsigned long i;
	mx_status_type mx_status;

#if MXI_XIA_XERXES_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	mx_status = mxi_xia_xerxes_get_pointers( mca,
					&xia_dxp_mca, &xia_xerxes, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: reading out baseline array from MCA '%s'.",
			fname, mca->record->name));
	}

#if MXI_XIA_XERXES_DEBUG_TIMING
	MX_HRT_START( measurement );
#endif

#if XIA_HAVE_OLD_DXP_READOUT_DETECTOR_RUN

	xia_status = dxp_readout_detector_run(
				&(xia_dxp_mca->detector_channel),
				NULL,
				xia_dxp_mca->xerxes_baseline_array,
				NULL );
#else

	xia_status = dxp_readout_detector_run(
				&(xia_dxp_mca->detector_channel),
				NULL,
				xia_dxp_mca->baseline_array,
				NULL );
#endif

#if MXI_XIA_XERXES_DEBUG_TIMING
	MX_HRT_END( measurement );

	MX_HRT_RESULTS( measurement, fname,
		"for record '%s'", mca->record->name );
#endif

	if ( xia_status != DXP_SUCCESS ) {
		return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
		"Cannot read the spectrum for XIA MCA '%s'.  "
		"Error code = %d, '%s'", mca->record->name,
			xia_status, mxi_xia_xerxes_strerror( xia_status ) );
	}

#if XIA_HAVE_OLD_DXP_READOUT_DETECTOR_RUN

	/* Copy the baseline array to the portable array. */

	for ( i = 0; i < xia_dxp_mca->baseline_length; i++ ) {
		xia_dxp_mca->baseline_array[i] = 
			(unsigned long) xia_dxp_mca->xerxes_baseline_array[i];
	}
#endif

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: readout from MCA '%s' complete.",
			fname, mca->record->name));
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_xia_xerxes_special_processing_setup( MX_RECORD *record )
{
	static const char fname[] = "mxi_xia_xerxes_special_processing_setup()";

	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	MX_DEBUG(2,("%s invoked.", fname));

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case MXLV_XIA_XERXES_CONFIG_FILENAME:
			record_field->process_function
					    = mxi_xia_xerxes_process_function;
			break;
		default:
			break;
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

static mx_status_type
mxi_xia_xerxes_read_dspsymbol_array( MX_XIA_DXP_MCA *xia_dxp_mca,
				char *dspsymbol_namestem,
				int num_array_elements,
				unsigned short *dspsymbol_array )
{
	static const char fname[] = "mxi_xia_xerxes_read_dspsymbol_array()";

	char dspsymbol_name[20];
	int i, xia_status;

#if MXI_XIA_XERXES_DEBUG_TIMING
	MX_HRT_TIMING measurement;
#endif

	for ( i = 0; i < num_array_elements; i++ ) {

		sprintf( dspsymbol_name, "%s%d", dspsymbol_namestem, i );

#if MXI_XIA_XERXES_DEBUG_TIMING
		MX_HRT_START( measurement );
#endif

		xia_status = dxp_get_one_dspsymbol(
					&(xia_dxp_mca->detector_channel),
					dspsymbol_name,
					&(dspsymbol_array[i]) );

#if MXI_XIA_XERXES_DEBUG_TIMING
		MX_HRT_END( measurement );

		MX_HRT_RESULTS( measurement, fname,
			"for record '%s', channel %d, parameter '%s'",
			xia_dxp_mca->record->name,
			xia_dxp_mca->detector_channel,
			dspsymbol_name );
#endif

		if ( xia_status != DXP_SUCCESS ) {
			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
			"Cannot read DXP parameter '%s' for detector "
			"channel %d of XIA DXP MCA '%s'.  "
			"Error code = %d, '%s'",
				dspsymbol_name, xia_dxp_mca->detector_channel,
				xia_dxp_mca->record->name, xia_status,
				mxi_xia_xerxes_strerror( xia_status ) );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

const char *
mxi_xia_xerxes_strerror( int xerxes_status )
{
	static struct {
		int xerxes_error_code;
		const char xerxes_error_string[30];
	} error_code_table[] = {
		{   0, "DXP_SUCCESS" },
		{   1, "DXP_MDOPEN" },
		{   2, "DXP_MDIO" },
		{   3, "DXP_MDINITIALIZE" },
		{   4, "DXP_MDLOCK" },
		{ 101, "DXP_WRITE_TSAR" },
		{ 102, "DXP_WRITE_CSR" },
		{ 103, "DXP_WRITE_WORD" },
		{ 104, "DXP_READ_WORD" },
		{ 105, "DXP_WRITE_BLOCK" },
		{ 106, "DXP_READ_BLOCK" },
		{ 107, "DXP_DISABLE_LAM" },
		{ 108, "DXP_CLR_LAM" },
		{ 109, "DXP_TEST_LAM" },
		{ 110, "DXP_READ_CSR" },
		{ 111, "DXP_WRITE_FIPPI" },
		{ 112, "DXP_WRITE_DSP" },
		{ 113, "DXP_WRITE_DATA" },
		{ 114, "DXP_READ_DATA" },
		{ 115, "DXP_ENABLE_LAM" },
		{ 116, "DXP_READ_GSR" },
		{ 117, "DXP_WRITE_GCR" },
		{ 118, "DXP_WRITE_WCR" },
		{ 119, "DXP_READ_WCR" },
		{ 201, "DXP_MEMERROR" },
		{ 202, "DXP_DSPRUNERROR" },
		{ 203, "DXP_FIPDOWNLOAD" },
		{ 204, "DXP_DSPDOWNLOAD" },
		{ 205, "DXP_INTERNAL_GAIN" },
		{ 206, "DXP_RESET_WARNING" },
		{ 207, "DXP_DETECTOR_GAIN" },
		{ 208, "DXP_NOSYMBOL" },
		{ 209, "DXP_SPECTRUM" },
		{ 210, "DXP_DSPLOAD" },
		{ 211, "DXP_DSPPARAMS" },
		{ 212, "DXP_DSPACCESS" },
		{ 213, "DXP_DSPPARAMBOUNDS" },
		{ 214, "DXP_ADC_RUNACTIVE" },
		{ 215, "DXP_ADC_READ" },
		{ 216, "DXP_ADC_TIMEOUT" },
		{ 217, "DXP_ALLOCMEM" },
		{ 218, "DXP_NOCONTROLTYPE" },
		{ 219, "DXP_NOFIPPI" },
		{ 301, "DXP_BAD_PARAM" },
		{ 302, "DXP_NODECIMATION" },
		{ 303, "DXP_OPEN_FILE" },
		{ 304, "DXP_NORUNTASKS" },
		{ 305, "DXP_OUTPUT_UNDEFINED" },
		{ 306, "DXP_INPUT_UNDEFINED" },
		{ 307, "DXP_ARRAY_TOO_SMALL" },
		{ 308, "DXP_NOCHANNELS" },
		{ 309, "DXP_NODETCHAN" },
		{ 310, "DXP_NOIOCHAN" },
		{ 311, "DXP_NOMODCHAN" },
		{ 312, "DXP_NEGBLOCKSIZE" },
		{ 313, "DXP_FILENOTFOUND" },
		{ 314, "DXP_NOFILETABLE" },
		{ 315, "DXP_INITIALIZE" },
		{ 316, "DXP_UNKNOWN_BTYPE" },
		{ 317, "DXP_NOMATCH" },
		{ 318, "DXP_BADCHANNEL" },
		{ 319, "DXP_DSPTIMEOUT" },
		{ 320, "DXP_INITORDER" },
		{ 401, "DXP_NOMEM" },
		{ 403, "DXP_CLOSE_FILE" },
		{ 404, "DXP_INDEXOOB" },
		{ 405, "DXP_RUNACTIVE" },
		{ 406, "DXP_MEMINUSE" },
		{1001, "DXP_DEBUG" }
	};

	static const char unrecognized_error_string[]
		= "Unrecognized XerXes error code";

	static int num_error_codes = sizeof( error_code_table )
				/ sizeof( error_code_table[0] );

	unsigned int i;

	for ( i = 0; i < num_error_codes; i++ ) {

		if ( xerxes_status == error_code_table[i].xerxes_error_code )
			break;		/* Exit the for() loop. */
	}

	if ( i >= num_error_codes ) {
		return &unrecognized_error_string[0];
	}

	return &( error_code_table[i].xerxes_error_string[0] );
}

#ifndef MX_PROCESS_GET

#define MX_PROCESS_GET	1
#define MX_PROCESS_PUT	2

#endif

static mx_status_type
mxi_xia_xerxes_process_function( void *record_ptr,
			void *record_field_ptr, int operation )
{
	static const char fname[] = "mxi_xia_xerxes_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_XIA_XERXES *xia_xerxes;
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	xia_xerxes = (MX_XIA_XERXES *) (record->record_type_struct);

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case MXLV_XIA_XERXES_CONFIG_FILENAME:
			/* Nothing to do since the necessary filename is
			 * already stored in the 'config_filename' field.
			 */

			break;
		default:
			MX_DEBUG( 1,(
			    "%s: *** Unknown MX_PROCESS_GET label value = %ld",
				fname, record_field->label_value));
			break;
		}
		break;
	case MX_PROCESS_PUT:
		switch( record_field->label_value ) {
		case MXLV_XIA_XERXES_CONFIG_FILENAME:
			mx_status = mxi_xia_xerxes_restore_config( xia_xerxes );

			break;
		default:
			MX_DEBUG( 1,(
			    "%s: *** Unknown MX_PROCESS_PUT label value = %ld",
				fname, record_field->label_value));
			break;
		}
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unknown operation code = %d", operation );
		break;
	}

	return mx_status;
}

#endif /* HAVE_XIA_XERXES */
