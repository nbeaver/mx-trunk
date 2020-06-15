/*
 * Name:    i_dante.c
 *
 * Purpose: MX driver for the XGLab DANTE controller.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2020 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_DANTE_DEBUG			TRUE

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>

/* Vendor include file. */

#include "DLL_DPP_Callback.h"

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_cfn.h"
#include "mx_array.h"
#include "mx_process.h"
#include "mx_mca.h"
#include "i_dante.h"
#include "d_dante_mca.h"

#if MXI_DANTE_DEBUG_TIMING
#  include "mx_hrt_debug.h"
#endif

MX_RECORD_FUNCTION_LIST mxi_dante_record_function_list = {
	mxi_dante_initialize_driver,
	mxi_dante_create_record_structures,
	mxi_dante_finish_record_initialization,
	NULL,
	NULL,
	mxi_dante_open,
	mxi_dante_close,
	mxi_dante_finish_delayed_initialization,
	NULL,
	mxi_dante_special_processing_setup
};

MX_RECORD_FIELD_DEFAULTS mxi_dante_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_DANTE_STANDARD_FIELDS
};

long mxi_dante_num_record_fields
		= sizeof( mxi_dante_record_field_defaults )
			/ sizeof( mxi_dante_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_dante_rfield_def_ptr
			= &mxi_dante_record_field_defaults[0];

static mx_status_type mxi_dante_process_function( void *record_ptr,
						void *record_field_ptr,
						void *socket_handler_ptr,
						int operation );

extern const char *mxi_dante_strerror( int xia_status );

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_dante_initialize_driver( MX_DRIVER *driver )
{
	static const char fname[] = "mxi_dante_initialize_driver()";

	MX_RECORD_FIELD_DEFAULTS *record_field_defaults;
	long num_active_detector_channels_varargs_cookie;
	MX_RECORD_FIELD_DEFAULTS *field;
	long referenced_field_index;
	mx_status_type mx_status;

	record_field_defaults = *(driver->record_field_defaults_ptr);

	if ( record_field_defaults == (MX_RECORD_FIELD_DEFAULTS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"'record_field_defaults' for record type '%s' is NULL.",
			driver->name );
	}

	if ( driver->num_record_fields == (long *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"'num_record_fields' pointer for record type '%s' is NULL.",
			driver->name );
	}

#if 0

	/* Get varargs cookie for 'num_active_detector_channels'. */

	mx_status = mx_find_record_field_defaults_index( driver,
						"num_active_detector_channels",
						&referenced_field_index );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_construct_varargs_cookie( referenced_field_index, 0,
				&num_active_detector_channels_varargs_cookie );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set varargs cookie for 'active_detector_channel_array'. */

	mx_status = mx_find_record_field_defaults( driver,
						"active_detector_channel_array",
						&field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	field->dimension[0] = num_active_detector_channels_varargs_cookie;
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_dante_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_dante_create_record_structures()";

	MX_DANTE *dante;

	/* Allocate memory for the necessary structures. */

	dante = (MX_DANTE *) malloc( sizeof(MX_DANTE) );

	if ( dante == (MX_DANTE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Cannot allocate memory for MX_DANTE structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = NULL;;
	record->record_type_struct = dante;

	record->class_specific_function_list = NULL;

	dante->record = record;

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_dante_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] = "mxi_dante_finish_record_initialization()";

	MX_DANTE *dante;

	dante = (MX_DANTE *) record->record_type_struct;

	if ( dante == (MX_DANTE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_DANTE pointer for record '%s' is NULL.",
			record->name );
	}

	/* Allocate space for the array of MCA record pointers. */

	dante->mca_record_array = (MX_RECORD **)
		calloc( dante->num_mcas, sizeof(MX_RECORD *) );

	if ( dante->mca_record_array == (MX_RECORD **) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory in record '%s' trying to allocate space "
		"for a %lu element array of MCA record pointers.",
			record->name, dante->num_mcas );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_dante_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_dante_open()";

	MX_DANTE *dante = NULL;
	long dimension[3];
	size_t dimension_sizeof[3];
	unsigned long i, j;
	unsigned long max_chain_boards;
	unsigned long flags;

	bool dante_status, show_devices, skip_board;
	uint32_t version_length;
	uint16_t number_of_devices;
	uint16_t board_number, max_identifier_length;
	uint16_t error_code = DLL_NO_ERROR;
	char identifier[MXU_DANTE_MAX_IDENTIFIER_LENGTH+1];

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	MX_DEBUG(-2,("%s invoked for '%s'.", fname, record->name));

	dante = (MX_DANTE *) record->record_type_struct;

	if ( dante == (MX_DANTE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_DANTE pointer for DANTE controller '%s' is NULL.",
			record->name );
	}

	/* Initialize the DANTE library. */

	dante_status = InitLibrary();

	if ( dante_status == false ) {
		return mx_error( MXE_UNKNOWN_ERROR, fname,
			"DANTE InitLibrary() failed." );
	}

	version_length = MXU_DANTE_MAX_VERSION_LENGTH;

	dante_status = libVersion( dante->dante_version, version_length );

	if ( dante_status == false ) {
		return mx_error( MXE_UNKNOWN_ERROR, fname,
			"DANTE libVersion() failed." );
	}

	MX_DEBUG(-2,("%s: DANTE version = '%s'", fname, dante->dante_version ));

	/* Find identifiers for all of the devices controlled by
	 * the current program.
	 */

	if ( dante->dante_flags & MXF_DANTE_SHOW_DEVICES ) {
		show_devices = true;
	} else {
		show_devices = false;
	}

	/* Wait a little while for daisy chain synchronization. */

	mx_sleep( 1 );

	/* How many masters are there? */

	dante_status = get_dev_number( number_of_devices );

	dante->num_master_devices = number_of_devices;

#if 0
	if ( show_devices ) {
		MX_DEBUG(-2,("%s: number of master devices = %lu",
			fname, dante->num_master_devices ));
	}
#endif

	/* Prepare for getting the identifiers of all boards available. */

	dante->num_boards_for_chain = (unsigned long *)
		calloc( dante->num_master_devices, sizeof(unsigned long) );

	if ( dante->num_boards_for_chain == (unsigned long *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %lu element array of "
		"unsigned longs for the field '%s.num_boards_per_chain'.",
			dante->num_master_devices, record->name );
	}

	MX_DEBUG(-2,("%s: dante->num_master_devices = %lu",
		fname, dante->num_master_devices));
	MX_DEBUG(-2,("%s: dante->max_boards_per_chain = %lu",
		fname, dante->max_boards_per_chain));

	dimension[0] = dante->num_master_devices;
	dimension[1] = dante->max_boards_per_chain;
	dimension[2] = MXU_DANTE_MAX_IDENTIFIER_LENGTH;

	dimension_sizeof[0] = sizeof(char);
	dimension_sizeof[1] = sizeof(char *);
	dimension_sizeof[2] = sizeof(char **);

	dante->board_identifier = (char ***)
	    mx_allocate_array( MXFT_STRING, 3, dimension, dimension_sizeof );

	if ( dante->board_identifier == (char ***) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a (%lu x %lu) element "
		"array of board identifiers for record '%s'.",
			dante->num_master_devices,
			dante->max_boards_per_chain,
			record->name );
	}

	/* How many boards are there for each master? */

	for ( i = 0; i < dante->num_master_devices; i++ ) {
	    dante->num_boards_for_chain[i] = dante->max_boards_per_chain;

	    for ( j = 0; j < dante->num_boards_for_chain[i]; j++ ) {

		if ( j == 0 ) {
			dante->board_identifier[i][j][0] = '\0';

			skip_board = false;
		}

		if ( skip_board ) {
			/* If we are skipping a board, we set the board's
			 * identifier to an empty string to indicate that
			 * this board does not exist.
			 */
			dante->board_identifier[i][j][0] = '\0';
			continue;
		}

		board_number = j;

#if 0
		MX_DEBUG(-2,("%s: master %lu, board %lu", fname, i, j ));
#endif

		error_code = DLL_NO_ERROR;

		max_identifier_length = MXU_DANTE_MAX_IDENTIFIER_LENGTH;

		(void) resetLastError();

		dante_status = get_ids( dante->board_identifier[i][j],
				board_number, max_identifier_length );

		if ( dante_status == false ) {
			(void) getLastError( error_code );

			switch( error_code ) {
			case DLL_NO_ERROR:
				break;
			case DLL_ARGUMENT_OUT_OF_RANGE:
				/* We have reached the end of this chain
				 * of boards.  Record the number of boards
				 * for this chain and then move on to the
				 * next chain.
				 */

				dante->num_boards_for_chain[i] = j+1;
				dante->board_identifier[i][j][0] = '\0';
				skip_board = true;
				break;

			default:
				return mx_error( MXE_UNKNOWN_ERROR, fname,
				"A call to get_ids() for record '%s' failed.  "
				"DANTE error code = %lu",
				    record->name, (unsigned long) error_code );
				break;
			}
		}

		if ( show_devices ) {
		    if ( skip_board == false ) {
			MX_DEBUG(-2,("%s: board[%lu][%lu] = '%s'",
				fname, i, j, dante->board_identifier[i][j] ));
		    }
		}
	    }
	}

	(void) resetLastError();

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_dante_close( MX_RECORD *record )
{
	static const char fname[] = "mxi_dante_close()";

	MX_DANTE *dante = NULL;
	bool dante_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	MX_DEBUG(-2,("%s invoked for '%s'.", fname, record->name ));

	dante = (MX_DANTE *) record->record_type_struct;

	if ( dante == (MX_DANTE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_DANTE pointer for DANTE controller '%s' is NULL.",
			record->name );
	}

	/* We must shut down the DANTE library before we close the
	 * MX 'dante' driver.  If we do not do this, then at shutdown
	 * time the MX program that was using this driver will _hang_
	 * in a call to exit().
	 */

	dante_status = CloseLibrary();

	if ( dante_status == false ) {
		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"The attempt to shutdown the DANTE library by calling "
		"CloseLibrary() failed.  But there is not really much "
		"that we can do at this point of the driver execution." );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_dante_finish_delayed_initialization( MX_RECORD *record )
{
	static const char fname[] = "mxi_dante_finish_delayed_initialization()";

	mx_status_type mx_status;

	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, record->name ));

	mx_status = mxi_dante_load_config_file( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_dante_configure( record );

	return mx_status;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_dante_special_processing_setup( MX_RECORD *record )
{
	static const char fname[] = "mxi_dante_special_processing_setup()";

	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	MX_DEBUG(-2,("%s invoked.", fname));

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case MXLV_DANTE_CONFIGURE:
		case MXLV_DANTE_LOAD_CONFIG_FILE:
		case MXLV_DANTE_SAVE_CONFIG_FILE:
			record_field->process_function
					    = mxi_dante_process_function;
			break;
		default:
			break;
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

static mx_status_type
mxi_dante_process_function( void *record_ptr,
			void *record_field_ptr,
			void *socket_handler_ptr,
			int operation )
{
	static const char fname[] = "mxi_dante_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_DANTE *dante;
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	dante = (MX_DANTE *) (record->record_type_struct);

	dante = dante;

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		default:
			MX_DEBUG( 1,(
			    "%s: *** Unknown MX_PROCESS_GET label value = %ld",
				fname, record_field->label_value));
			break;
		}
		break;
	case MX_PROCESS_PUT:
		switch( record_field->label_value ) {
		case MXLV_DANTE_CONFIGURE:
			mx_status = mxi_dante_configure( record );
			break;
		case MXLV_DANTE_LOAD_CONFIG_FILE:
			mx_status = mxi_dante_load_config_file( record );
			break;
		case MXLV_DANTE_SAVE_CONFIG_FILE:
			mx_status = mxi_dante_save_config_file( record );
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

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_dante_configure( MX_RECORD *record )
{
	static const char fname[] = "mxi_dante_configure()";

	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, record->name));

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_dante_load_config_file( MX_RECORD *record )
{
	static const char fname[] = "mxi_dante_load_config_file()";

	MX_DANTE *dante = NULL;
	MX_RECORD *current_mca_record = NULL;
	char current_mca_identifier[MXU_DANTE_MAX_IDENTIFIER_LENGTH+1];
	char mca_name_format[40];
	FILE *config_file = NULL;
	char *ptr = NULL;
	int saved_errno;
	char buffer[200];

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	MX_DEBUG(-2,("%s invoked for '%s'.", fname, record->name));

	dante = (MX_DANTE *) record->record_type_struct;

	if ( dante == (MX_DANTE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_DANTE pointer for DANTE controller '%s' is NULL.",
			record->name );
	}

	MX_DEBUG(-2,("%s: DANTE record '%s' will load parameters "
		"from config file '%s'.",
			fname, record->name, dante->config_filename ));

	/* NOTE: We are reading the XML by hand, in order to avoid
	 * having to find an XML library with an appropriate license
	 * to link to for this.  If we need to do much more XML
	 * processing in the future, we may want to revisit 
	 * this choice here.
	 */

	/* FIXME: fopen() seems to work differently in C++, with the
	 * result that it is impossible to read strings from the file
	 * using mx_fgets().  By using mx_cfn_fopen(), we use a copy
	 * of fopen() in the C library libMx.  That seems to work
	 * for some reason.  Perhaps some sort of memory corruption
	 * is occurring?
	 */
#if 0
	config_file = fopen( dante->config_filename, "r" );
#else
	config_file = mx_cfn_fopen( MX_CFN_ABSOLUTE,
			dante->config_filename, "r" );
#endif

	MX_DEBUG(-2,("%s: config_file = %p", fname, config_file ));

	if ( config_file == (FILE *) NULL ) {
		saved_errno = errno;

		switch( saved_errno ) {
		case EACCES:
			return mx_error( MXE_PERMISSION_DENIED, fname,
			"You do not have permission to read "
			"DANTE configuration file '%s' for record '%s'.",
				dante->config_filename, record->name );
			break;
		case ENOENT:
			return mx_error( MXE_NOT_FOUND, fname,
			"DANTE configuration file '%s' cannot be found "
			"for record '%s'.",
				dante->config_filename, record->name );
			break;
		default:
			return mx_error( MXE_FILE_IO_ERROR, fname,
			"Cannot open DANTE configuration file '%s' "
			"for record '%s'.  Errno = %d, error message = '%s'.",
				dante->config_filename, record->name,
				saved_errno, strerror(saved_errno) );
			break;
		}
	}

	/*  Work our way through the XML file, one line at a time. */

	snprintf( mca_name_format, sizeof(mca_name_format),
		"<DPP Name=\"%%%ds\"", sizeof(current_mca_identifier) );

	current_mca_identifier[0] = '\0';

	current_mca_record = NULL;

	while (TRUE) {
	    /* Read one line from the XML file. */

	    MX_DEBUG(-2,("%s: About to read buffer.", fname));

	    ptr = mx_fgets( buffer, sizeof(buffer), config_file );

	    MX_DEBUG(-2,("%s: ptr = %p", fname, ptr));

	    if ( ptr == (char *) NULL ) {

		if ( feof(config_file) ) {
		    MX_DEBUG(-2,("%s: Reached end of file.", fname));

		    break;	/* Exit the while loop. */
		}

		if ( ferror(config_file) ) {
		    saved_errno = errno;
		    MX_DEBUG(-2,
			("%s: Error reading config file '%s' occurred.  "
			"Errno = %d, error_message = '%s'",
				fname, dante->config_filename,
				saved_errno, strerror(saved_errno) ));

		    break;	/* Exit the while loop. */
		}
	    }

	    MX_DEBUG(-2,("%s: buffer = '%s'", fname, buffer));

	    ptr = strstr( buffer, "<DPP Name" );

	    if ( ptr != (char *) NULL ) {
		num_items = sscanf( buffer, mca_name_format,
					current_mca_identifier );

		if ( num_items == 1 ) {
		    /* current_mca_identifier may contain the name of an MCA.
		     * So we go look for a match in dante->mca_record_array.
		     */

		    current_mca_record = NULL;
		    current_dante_mca = NULL;

		    for ( i = 0; i < dante->num_mcas, i++ ) {
			mca_record = dante->mca_record_array[i];

			if ( mca_record == (MX_RECORD *) NULL ) {
			    continue;	/* Cycle the for(i) loop. */
			}

			dante_mca = (MX_DANTE_MCA *)
					mca_record->record_type_struct;

			if ( dante_mca == (MX_DANTE_MCA *) NULL ) {
			    return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			    "The MX_DANTE_MCA pointer for DANTE MCA '%s' "
			    "is NULL.", mca_record->name );
			}

			/* Is this MCA the one we are looking for? */

			if ( strcmp( current_mca_identifier,
					dante_mca->channel_name ) != 0 )
			{
			    /* This is not the right MCA, so go on
			     * to the next one.
			     */

			    continue;	/* Cycle the for(i) loop. */
			}

			/* We have found the matching MCA, so save that fact. */

		    	current_mca_record = mca_record;
			current_dante_mca = dante_mca;

			MX_DEBUG(-2,("%s: current_mca_record = '%s', "
				"current_dante_mca->channel_name = '%s'.",
				current_mca_record->name,
				current_dante_mca->channel_name ));
		    }

		} else {
		    /* See if this line contains an MCA parameter name. */

		}
	    }
	}

	/* FIXME: fclose() below crashes, so we ifdef it out for now.
	 * But we need to really understand why it is crashing, since
	 * by commenting it out here, we create a memory leak.  This
	 * might be related to the problem above with fopen().
	 */

#if 0
	fclose( config_file );
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_dante_save_config_file( MX_RECORD *record )
{
	static const char fname[] = "mxi_dante_save_config_file()";

	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, record->name));

	return MX_SUCCESSFUL_RESULT;
}

