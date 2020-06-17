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

	MX_DANTE *dante = NULL;
	MX_RECORD *mca_record = NULL;
	unsigned long i;
	mx_status_type mx_status;

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

	mx_status = mxi_dante_load_config_file( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	for ( i = 0; i < dante->num_mcas; i++ ) {

		mca_record = dante->mca_record_array[i];

		if ( mca_record == (MX_RECORD *) NULL ) {
			continue;	/* Iterate the for(i) loop. */
		}

		mx_status = mxi_dante_show_parameters( mca_record );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

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

	MX_DANTE *dante = NULL;
	MX_RECORD *mca_record = NULL;
	MX_DANTE_MCA *dante_mca = NULL;
	unsigned long i;
	uint32_t call_id;
	mx_status_type mx_status;

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

	for ( i = 0; i < dante->num_mcas; i++ ) {

		mca_record = dante->mca_record_array[i];

		if ( mca_record == (MX_RECORD *) NULL ) {
			continue;	/* Iterate the for(i) loop. */
		}

		dante_mca = (MX_DANTE_MCA *) mca_record->record_type_struct;

		if ( dante_mca == (MX_DANTE_MCA *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_DANTE_MCA pointer for MCA record '%s' is NULL.",
				mca_record->name );
		}

		MX_DEBUG(-2,("%s: About to configure DANTE MCA '%s'.",
			fname, mca_record->name ));

		call_id = configure( dante_mca->channel_name,
					dante_mca->board_number,
					dante_mca->configuration );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

static mx_status_type
mxi_dante_set_parameter_from_string( MX_DANTE_MCA *dante_mca,
					char *parameter_name,
					char *parameter_string )
{
	static const char fname[] = "mxi_dante_set_parameter_from_string()";

	if ( dante_mca == (MX_DANTE_MCA *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DANTE_MCA pointer passed was NULL." );
	}

	MX_DEBUG(-2,("  '%s' parameter_name = '%s', parameter_string = '%s'.",
		dante_mca->record->name, parameter_name, parameter_string));

	struct configuration *configuration = &(dante_mca->configuration);

	/* Look through the known parameter names */

	if ( strcmp( parameter_name, "fast_filter_thr" ) == 0 ) {
		MX_DEBUG(-2,("**** %s: '%s' dante_mca = %p, configuration = %p",
			fname, dante_mca->record->name,
			dante_mca, configuration));

		configuration->fast_filter_thr = atol( parameter_string );

	} else if ( strcmp( parameter_name, "energy_filter_thr" ) == 0 ) {
		configuration->energy_filter_thr = atol( parameter_string );

	} else if ( strcmp( parameter_name, "energy_baseline_thr" ) == 0 ) {
		configuration->energy_baseline_thr = atol( parameter_string );

	} else if ( strcmp( parameter_name, "max_risetime" ) == 0 ) {
		configuration->max_risetime = atof( parameter_string );

	} else if ( strcmp( parameter_name, "gain" ) == 0 ) {
		configuration->gain = atof( parameter_string );

	} else if ( strcmp( parameter_name, "peaking_time" ) == 0 ) {
		configuration->peaking_time = atol( parameter_string );

	} else if ( strcmp( parameter_name, "max_peaking_time" ) == 0 ) {
		configuration->max_peaking_time = atol( parameter_string );

	} else if ( strcmp( parameter_name, "flat_top" ) == 0 ) {
		configuration->flat_top = atol( parameter_string );

	} else if ( strcmp( parameter_name, "edge_peaking_time" ) == 0 ) {
		configuration->edge_peaking_time = atol( parameter_string );

	} else if ( strcmp( parameter_name, "edge_flat_top" ) == 0 ) {
		configuration->edge_flat_top = atol( parameter_string );

	} else if ( strcmp( parameter_name, "reset_recovery_time" ) == 0 ) {
		configuration->reset_recovery_time = atol( parameter_string );

	} else if ( strcmp( parameter_name, "zero_peak_freq" ) == 0 ) {
		configuration->zero_peak_freq = atof( parameter_string );

	} else if ( strcmp( parameter_name, "baseline_samples" ) == 0 ) {
		configuration->baseline_samples = atol( parameter_string );

	} else if ( strcmp( parameter_name, "inverted_input" ) == 0 ) {
		long temp_long = atol( parameter_string );

		if ( temp_long == 0 ) {
			configuration->inverted_input = false;
		} else {
			configuration->inverted_input = true;
		}

	} else if ( strcmp( parameter_name, "time_constant" ) == 0 ) {
		configuration->time_constant = atof( parameter_string );

	} else if ( strcmp( parameter_name, "base_offset" ) == 0 ) {
		configuration->base_offset = atol( parameter_string );

	} else if ( strcmp( parameter_name, "overflow_recovery" ) == 0 ) {
		configuration->overflow_recovery = atol( parameter_string );

	} else if ( strcmp( parameter_name, "reset_threshold" ) == 0 ) {
		configuration->reset_threshold = atol( parameter_string );

	} else if ( strcmp( parameter_name, "tail_coefficient" ) == 0 ) {
		configuration->tail_coefficient = atof( parameter_string );

	} else if ( strcmp( parameter_name, "other_param" ) == 0 ) {
		configuration->other_param = atol( parameter_string );

	} else {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unrecognized parameter seen for DANTE MCA '%s'.  "
		"parameter_name = '%s', parameter_string = '%s'.",
			dante_mca->record->name,
			parameter_name, parameter_string );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_dante_show_parameters( MX_RECORD *record )
{
	static const char fname[] = "mxi_dante_show_parameters()";

	MX_DANTE_MCA *dante_mca = (MX_DANTE_MCA *) record->record_type_struct;

	struct configuration *configuration = &(dante_mca->configuration);

	MX_DEBUG(-2,("**** %s: '%s' dante_mca = %p, configuration = %p",
		fname, dante_mca->record->name,
		dante_mca, configuration));

	MX_DEBUG(-2,("  fast_filter_thr = %lu",
		configuration->fast_filter_thr ));

	MX_DEBUG(-2,("  energy_filter_thr = %lu",
		configuration->energy_filter_thr ));

	MX_DEBUG(-2,("  energy_baseline_thr = %lu",
		configuration->energy_baseline_thr ));

	MX_DEBUG(-2,("  max_risetime = %g", configuration->max_risetime ));

	MX_DEBUG(-2,("  gain = %g", configuration->gain ));

	MX_DEBUG(-2,("  peaking_time = %lu", configuration->peaking_time ));

	MX_DEBUG(-2,("  max_peaking_time = %lu",
		configuration->max_peaking_time ));

	MX_DEBUG(-2,("  flat_top = %lu", configuration->flat_top ));

	MX_DEBUG(-2,("  edge_peaking_time = %lu",
		configuration->edge_peaking_time ));

	MX_DEBUG(-2,("  edge_flat_top = %lu", configuration->edge_flat_top ));

	MX_DEBUG(-2,("  reset_recovery_time = %lu",
		configuration->reset_recovery_time ));

	MX_DEBUG(-2,("  zero_peak_freq = %g", configuration->zero_peak_freq));

	MX_DEBUG(-2,("  baseline_samples = %lu",
		configuration->baseline_samples ));

	if ( configuration->inverted_input ) {
		MX_DEBUG(-2,("  inverted_input = true", fname ));
	} else {
		MX_DEBUG(-2,("  inverted_input = false", fname ));
	}

	MX_DEBUG(-2,("  time_constant = %g", configuration->time_constant ));

	MX_DEBUG(-2,("  base_offset = %lu",
		configuration->base_offset ));

	MX_DEBUG(-2,("  overflow_recovery = %lu",
		configuration->overflow_recovery ));

	MX_DEBUG(-2,("  reset_threshold = %lu",
		configuration->reset_threshold ));

	MX_DEBUG(-2,("  tail_coefficient = %g",
		configuration->tail_coefficient )); 

	MX_DEBUG(-2,("  other_param = %lu", configuration->other_param ));

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_dante_load_config_file( MX_RECORD *record )
{
	static const char fname[] = "mxi_dante_load_config_file()";

	MX_DANTE *dante = NULL;
	MX_RECORD *current_mca_record = NULL;
	MX_DANTE_MCA *current_dante_mca = NULL;
	char current_mca_identifier[MXU_DANTE_MAX_IDENTIFIER_LENGTH+1];
	MX_RECORD *mca_record = NULL;
	MX_DANTE_MCA *dante_mca = NULL;
	char mca_name_format[40];
	FILE *config_file = NULL;
	char *ptr = NULL;
	char *identifier_ptr = NULL;
	char *end_ptr = NULL;
	char *parameter_name = NULL;
	char *parameter_string = NULL;
	char *parameter_end = NULL;
	long i;
	int saved_errno;
	char buffer[200];
	mx_status_type mx_status;

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

	/* FIXME???: We are reading the XML by hand, in order to avoid having
	 * to find an XML library with an appropriate license to link to for
	 * this.  If we need to do much more XML processing in the future
	 * elsewhere in MX, we may want to revisit this choice here.
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
		" <DPP Name=\"%%%ds\">", sizeof(current_mca_identifier) );

#if 0
	MX_DEBUG(-2,("%s: mca_name_format = '%s'", fname, mca_name_format));
#endif

	current_mca_identifier[0] = '\0';

	current_mca_record = NULL;

	while (TRUE) {
	    /* Read one line from the XML file. */

	    ptr = mx_fgets( buffer, sizeof(buffer), config_file );

	    if ( ptr == (char *) NULL ) {

		if ( feof(config_file) ) {
#if 0
		    MX_DEBUG(-2,("%s: Reached end of file.", fname));
#endif

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

#if 0
	    MX_DEBUG(-2,("%s: buffer = '%s'", fname, buffer));
#endif

	    ptr = strstr( buffer, "<DPP Name" );

	    if ( ptr != (char *) NULL ) {
		/* This line appears to have an MCA identifier in it. */

		identifier_ptr = strstr( ptr, "\"" );

		if ( identifier_ptr != NULL ) {
		    identifier_ptr++;

		    end_ptr = strstr( identifier_ptr, "\"" );

		    if ( end_ptr != NULL ) {
			*end_ptr = '\0';
		    }
		}

		if ( identifier_ptr != NULL ) {
		    /* identifier_ptr may contain the name of an MCA.
		     * So we go look for a match in dante->mca_record_array.
		     */

		    current_mca_record = NULL;
		    current_dante_mca = NULL;

		    for ( i = 0; i < dante->num_mcas; i++ ) {
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

#if 0
			MX_DEBUG(-2,("%s: identifier_ptr = '%s'\n",
				fname, identifier_ptr));

			for (n = 0; n < strlen(identifier_ptr); n++ ) {
				fprintf( stderr, "%#x (%c) ",
					identifier_ptr[n],
					identifier_ptr[n]);
			}

			MX_DEBUG(-2,("%s: dante_mca->channel_name = '%s'\n",
				fname, dante_mca->channel_name));

			for (n = 0; n < strlen(dante_mca->channel_name); n++ ) {
				fprintf( stderr, "%#x (%c) ",
					dante_mca->channel_name[n],
					dante_mca->channel_name[n]);
			}
			fprintf(stderr,"\n" );
			fflush(stderr);
#endif

			if ( strcmp( identifier_ptr,
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
				fname, current_mca_record->name,
				current_dante_mca->channel_name ));

		    } /* End of the for(i) loop. */
		}
	    } else
	    if ( current_dante_mca == (MX_DANTE_MCA *) NULL ) {
#if 0
		MX_DEBUG(-2,("%s: current_dante_mca = NULL", fname));
#endif
	    } else {
		/* See if this line contains an MCA parameter name. */

		/* Look for the first '<' character on the line. */

		parameter_name = strchr( buffer, '<' );

		if ( parameter_name != (char *) NULL ) {
		    parameter_name++;

		    parameter_string = strchr( buffer, '>' );

		    if ( parameter_string != (char *) NULL ) {
			*parameter_string = '\0';

			parameter_string++;

			parameter_end = strchr( parameter_string, '<' );

			if ( parameter_end != (char *) NULL ) {
				*parameter_end = '\0';
			}

			mx_status = mxi_dante_set_parameter_from_string(
					current_dante_mca,
					parameter_name,
					parameter_string );
		    }
		}
	    }

	} /* End of the while(TRUE) loop. */

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

