/*
 * Name:    i_dante.cpp
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

#define MXI_DANTE_DEBUG_SHOW_PARAMETERS_FOR_CHAIN	TRUE

#define MXI_DANTE_DEBUG_CALLBACKS			FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

/* Vendor include file. */

#include "DLL_DPP_Callback.h"

#ifdef POLLINGLIB
#error Only the Callback mode library is supported.
#endif

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_cfn.h"
#include "mx_array.h"
#include "mx_process.h"
#include "mx_mcs.h"
#include "mx_mca.h"
#include "i_dante.h"
#include "d_dante_mcs.h"
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

/* FIXME: The following set of callback-related functions basically serialize
 * the handling of callbacks.  If we find ourself needing to deserialize the
 * handling of callbacks, then we will need to replace the following code.
 */

uint32_t mxi_dante_callback_id;

uint32_t mxi_dante_callback_data[MXU_DANTE_MAX_CALLBACK_DATA_LENGTH];

static void
mxi_dante_callback_fn( uint16_t type,
			uint32_t call_id,
			uint32_t length,
			uint32_t *data )
{
#if MXI_DANTE_DEBUG_CALLBACKS
	static const char fname[] = "mxi_dante_callback_fn()";

	uint32_t i;
#endif

#if MXI_DANTE_DEBUG_CALLBACKS
	fprintf( stderr,
	"\n>>> %s invoked.  type = %lu, call_id = %lu, length = %lu, "
	"mxi_dante_callback_data = ",
		fname, (unsigned long) type,
		(unsigned long) call_id, (unsigned long) length);
#endif

	if ( length > MXU_DANTE_MAX_CALLBACK_DATA_LENGTH ) {
		length = MXU_DANTE_MAX_CALLBACK_DATA_LENGTH;

		fprintf( stderr, "Shortening callback data to %lu values.\n",
					(unsigned long) length );
	}

	memcpy( mxi_dante_callback_data, data, length );

#if MXI_DANTE_DEBUG_CALLBACKS
	for ( i = 0; i < length; i++ ) {
		fprintf( stderr, "%lu ",
		(unsigned long) mxi_dante_callback_data[i] );
	}

	fprintf( stderr, "\n" );
	fflush( stderr );
#endif

	mxi_dante_callback_id = 0;

	return;
}

int
mxi_dante_wait_for_answer( uint32_t call_id )
{
	static const char fname[] = "mxi_dante_wait_for_answer()";

	int i;

	mxi_dante_callback_id = call_id;

#if MXI_DANTE_DEBUG_CALLBACKS
	MX_DEBUG(-2,("%s: Waiting for callback %lu",
		fname, (unsigned long) call_id));
#endif

	for ( i = 0; i < 10; i++ ) {
		if ( mxi_dante_callback_id != call_id ) {

#if MXI_DANTE_DEBUG_CALLBACKS
			MX_DEBUG(-2,("Callback %lu seen.",
				(unsigned long) call_id ));
#endif

			return TRUE;
		}

		mx_msleep(1000);
	}

	if ( i >= 10 ) {
		MX_DEBUG(-2,(
		"%s: Timed out waiting for callback to arrive.\n", fname ));
	}

	return FALSE;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_dante_initialize_driver( MX_DRIVER *driver )
{
#if 0
	static const char fname[] = "mxi_dante_initialize_driver()";

	MX_RECORD_FIELD_DEFAULTS *record_field_defaults;

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
	unsigned long i, attempt;

	unsigned long major, minor, update, extra;
	int num_items;

	unsigned long max_init_delay_ms;
	unsigned long max_attempts;

	bool dante_status, show_devices;
	uint32_t version_length;
	uint16_t number_of_devices;
	uint16_t board_number, max_identifier_length;
	uint16_t error_code = DLL_NO_ERROR;
	uint16_t num_boards;

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

	dante->dante_mode = MXF_DANTE_NORMAL_MODE;

	/* Set up an event callback for the DANTE library.  Please notice
	 * that this _MUST_ be done _before_ the call to InitLibrary().
	 */

	dante_status = register_callback( mxi_dante_callback_fn );

	if ( dante_status == false ) {
		getLastError( error_code );

		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"The attempt to register mxi_dante_callback_fn() as "
		"an event callback for DANTE has failed with "
		"error code %lu.", (unsigned long) error_code );
	}

	/* Initialize the DANTE library. */

	(void) resetLastError();

	dante_status = InitLibrary();

	if ( dante_status == false ) {
		(void ) getLastError( error_code );

		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"DANTE InitLibrary() failed with error code %lu.",
			(unsigned long) error_code );
	}

	/* Get the DANTE API version number. */

	version_length = MXU_DANTE_MAX_VERSION_LENGTH;

	dante_status = libVersion( dante->dante_version_string,
						version_length );

	if ( dante_status == false ) {
		(void ) getLastError( error_code );

		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"DANTE libVersion() failed with error code %lu.",
			(unsigned long) error_code );
	}

	num_items = sscanf( dante->dante_version_string, "%lu.%lu.%lu.%lu",
					&major, &minor, &update, &extra );

	switch( num_items ) {
	case 3:
		extra = 0;
		/* Fall through to case 4 */
	case 4:
		break;
	default:
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
		"Error in parsing the DANTE API version from string '%s'.  "
		"The DANTE API version is expected to have a format of "
		"w.x.y.z or w.x.y", dante->dante_version_string );
		break;
	}

	dante->dante_version = MX_DANTE_VERSION( major, minor, update, extra );

	MX_DEBUG(-2,("%s: DANTE version = '%s' (%lu)",
		fname, dante->dante_version_string, dante->dante_version ));

	if ( dante->dante_version < MX_DANTE_MIN_VERSION ) {
		unsigned long min_ver = MX_DANTE_MIN_VERSION;

		unsigned long min_major = ( min_ver / 1000000L );
		unsigned long min_minor = ( ( min_ver % 1000000L ) / 10000L );
		unsigned long min_update = ( ( min_ver % 10000L ) / 100L );
		unsigned long min_extra = ( min_ver % 100L );

		mx_warning( "The MX 'xglab_dante' module has not been "
		"tested with versions of DANTE API older than "
		"%lu.%lu.%lu.%lu.  Things may not work correctly "
		"for this version.\n",
			min_major, min_minor, min_update, min_extra);
	}

	if ( dante->dante_version > MX_DANTE_MAX_VERSION ) {
		unsigned long max_ver = MX_DANTE_MAX_VERSION;

		unsigned long max_major = ( max_ver / 1000000L );
		unsigned long max_minor = ( ( max_ver % 1000000L ) / 10000L );
		unsigned long max_update = ( ( max_ver % 10000L ) / 100L );
		unsigned long max_extra = ( max_ver % 100L );

		mx_warning( "The MX 'xglab_dante' module has not been "
		"tested with versions of DANTE API newer than "
		"%lu.%lu.%lu.%lu.  Things may not work correctly "
		"for this version.\n",
			max_major, max_minor, max_update, max_extra);
	}

	/* Find identifiers for all of the devices controlled by
	 * the current program.
	 */

	if ( dante->dante_flags & MXF_DANTE_SHOW_DEVICES ) {
		show_devices = true;
	} else {
		show_devices = false;
	}

	/* Wait a little while for daisy chain synchronization. */

	max_init_delay_ms = mx_round( 1000.0 * dante->max_init_delay );

#if 1
	MX_DEBUG(-2,("%s: Waiting %f seconds for boards to be found.",
		fname, dante->max_init_delay));
#endif

	mx_msleep( max_init_delay_ms );

	/* How many masters are there? */

	dante_status = get_dev_number( number_of_devices );

	if ( dante_status == false ) {
		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"DANTE get_dev_number() failed." );
	}

	dante->num_master_devices = number_of_devices;

	/* Allocate an array of chain master structures. */

	dante->master = (MX_DANTE_CHAIN *)
		calloc( number_of_devices, sizeof(MX_DANTE_CHAIN) );

	if ( dante->master == (MX_DANTE_CHAIN *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %lu element array of "
		"MX_DANTE_CHAIN structures for Dante record '%s'.",
			dante->num_master_devices, record->name );
	}

	MX_DEBUG(-2,("%s: dante->num_master_devices = %lu",
		fname, dante->num_master_devices));
	MX_DEBUG(-2,("%s: dante->max_boards_per_chain = %lu",
		fname, dante->max_boards_per_chain));

	/* How many boards are there for each master? */

	max_attempts = mx_round_up( dante->max_board_delay );

#if 1
	MX_DEBUG(-2,("%s: max_attempts = %lu", fname, max_attempts));
#endif

	for ( i = 0; i < dante->num_master_devices; i++ ) {
#if 1
		MX_DEBUG(-2,("%s: master %lu", fname, i ));
#endif

		error_code = DLL_NO_ERROR;

		max_identifier_length = MXU_DANTE_MAX_CHAIN_ID_LENGTH;

		(void) resetLastError();

		dante->master[i].chain_id[0] = '\0';

		board_number = 0;

		dante_status = get_ids( dante->master[i].chain_id,
					board_number, max_identifier_length );

		if ( dante_status == false ) {
			(void) getLastError( error_code );

			switch( error_code ) {
			case DLL_NO_ERROR:
				break;
			default:
				return mx_error( MXE_UNKNOWN_ERROR, fname,
				"A call to get_ids() for record '%s' failed.  "
				"DANTE error code = %lu",
				    record->name, (unsigned long) error_code );
				break;
			}
		}

#if 1
		MX_DEBUG(-2,("%s: dante->master[%lu].chain_id = '%s'",
			fname, i, dante->master[i].chain_id ));
#endif

		for ( attempt = 0; attempt < max_attempts; attempt++ ) {

			dante_status = get_boards_in_chain(
				dante->master[i].chain_id, num_boards );

			if ( dante_status == false ) {
				(void) getLastError( error_code );

				switch( error_code ) {
				case DLL_NO_ERROR:
				    break;
				default:
				    return mx_error( MXE_UNKNOWN_ERROR, fname,
				  "A call to get_boards_in_chain() for "
				  "record '%s' failed.  DANTE error code = %lu",
				    record->name, (unsigned long) error_code );
				    break;
				}
			}

			MX_DEBUG(-2,("%s: num_boards = %lu",
				fname, (unsigned long) num_boards));

			if ( num_boards > dante->max_boards_per_chain ) {
				return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
				"While setting up DANTE master '%s', we "
				"Found more boards (%lu) for chain '%s' than "
				"the maximum number (%lu) that we are prepared "
				"to handle.  Please increase the size of "
				"'%s.max_boards_per_chain' in the MX database.",
					dante->record->name,
					(unsigned long) num_boards,
					dante->master[i].chain_id,
					dante->max_boards_per_chain,
					dante->record->name );
			} else
			if ( num_boards >= dante->max_boards_per_chain ) {
				MX_DEBUG(-2,("%s: All boards found for '%s'.",
				fname, dante->master[i].chain_id ));

				break;
			}

			mx_msleep(1000);
		}

		dante->master[i].num_boards = num_boards;

#if 0
		if ( show_devices ) {
#else
		if ( 1 ) {
#endif
			MX_DEBUG(-2,("%s: board[%lu] = '%s' (%lu boards)",
				fname, i, dante->master[i].chain_id,
				dante->master[i].num_boards ));
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
		"CloseLibrary() failed.  Seemingly there is not really "
		"much that we can do about this.  But please report it." );
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
	mx_bool_type show_devices;
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

	mx_status = mxi_dante_show_parameters_for_chain( record );

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

	MXW_UNUSED( dante );

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

		mx_status = mxd_dante_mca_configure( dante_mca, NULL );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_dante_set_configuration_to_defaults(
		MX_DANTE_CONFIGURATION *mx_dante_configuration,
		char *configuration_chain_name )
{
	static const char fname[] = "mxi_dante_set_configuration_to_defaults()";

	if ( mx_dante_configuration == (MX_DANTE_CONFIGURATION *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DANTE_CONFIGURATION pointer passed was NULL." );
	}

	if ( configuration_chain_name == (char *) NULL ) {
		mx_dante_configuration->chain_name[0] = '\0';
	} else {
		strlcpy( mx_dante_configuration->chain_name,
				configuration_chain_name,
				sizeof(mx_dante_configuration->chain_name) );
	}

	/* Initialize the configuration parameters to the values that
	 * are set as the defaults in the vendor's header file.
	 */

	struct configuration *test_config = new struct configuration;

	memcpy( &(mx_dante_configuration->configuration), test_config,
					sizeof(struct configuration) );

	delete test_config;

	mx_dante_configuration->input_mode = DC_HighImp;	/* is 0 */
	mx_dante_configuration->gating_mode = FreeRunning;	/* is 0 */

	mx_dante_configuration->timestamp_delay = 0;
	mx_dante_configuration->baseline_offset = 0;

	mx_dante_configuration->offset[0] = 0;
	mx_dante_configuration->offset[1] = 0;

	mx_dante_configuration->calib_energies_bins[0] = 0;
	mx_dante_configuration->calib_energies_bins[1] = 0;

	mx_dante_configuration->calib_energies[0] = 0.0;
	mx_dante_configuration->calib_energies[1] = 0.0;

	mx_dante_configuration->calib_channels = 0;

	mx_dante_configuration->calib_equation = 0;

	mx_dante_configuration->configuration_flags = 
				MXF_DANTE_CONFIGURATION_DEBUG_PARAMETERS;

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

static mx_status_type
mxi_dante_set_parameter_from_string(
		MX_DANTE_CONFIGURATION *mx_dante_configuration,
					char *parameter_name,
					char *parameter_string )
{
	static const char fname[] = "mxi_dante_set_parameter_from_string()";

	int num_items;
	unsigned long flags;
	mx_bool_type debug_parameters;

	if ( mx_dante_configuration == (MX_DANTE_CONFIGURATION  *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DANTE_CONFIGURATION pointer passed was NULL." );
	}

	struct configuration *configuration =
		&(mx_dante_configuration->configuration);

	flags = mx_dante_configuration->configuration_flags;

	if ( flags & MXF_DANTE_CONFIGURATION_DEBUG_PARAMETERS) {
		debug_parameters = TRUE;
	} else {
		debug_parameters = FALSE;
	}

	if ( debug_parameters ) {
		fprintf( stderr, "  parameter '%s', value = '%s'",
				parameter_name, parameter_string );
	}

	/* Skip over any parameters that are set to 'null' values. */

	if ( strcmp( parameter_string, "null" ) == 0 ) {

		if ( debug_parameters ) {
			fprintf( stderr, " ... skipping.\n" );
		}

		return MX_SUCCESSFUL_RESULT;
	}

	/* Look through the known parameter names */

	if ( strcmp( parameter_name, "fast_filter_thr" ) == 0 ) {
		configuration->fast_filter_thr = atol( parameter_string );

	} else if ( strcmp( parameter_name, "energy_filter_thr" ) == 0 ) {
		configuration->energy_filter_thr = atol( parameter_string );

	} else if ( strncmp( parameter_name, "energy_baseline_thr",
			  strlen("energy_baseline_thr")  ) == 0 )
	{
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

	} else if ( strcmp( parameter_name, "input_mode" ) == 0 ) {
		mx_dante_configuration->input_mode =
			(InputMode) atol( parameter_string );

	} else if ( strcmp( parameter_name, "TimestampDelay" ) == 0 ) {
		mx_dante_configuration->timestamp_delay =
						atol( parameter_string );

	} else if ( strcmp( parameter_name, "baseline_offset" ) == 0 ) {
		mx_dante_configuration->baseline_offset =
						atol( parameter_string );

	} else if ( strcmp( parameter_name, "Offset" ) == 0 ) {
		mx_dante_configuration->offset[0] = atol( parameter_string );

	} else if ( strcmp( parameter_name, "offset_1" ) == 0 ) {
		mx_dante_configuration->offset[0] = atol( parameter_string );

	} else if ( strcmp( parameter_name, "offset_2" ) == 0 ) {
		mx_dante_configuration->offset[1] = atol( parameter_string );

	} else if ( strcmp( parameter_name, "calib_energies_bins" ) == 0 ) {
		num_items = sscanf( parameter_string, "%lu;%lu",
			&(mx_dante_configuration->calib_energies_bins[0]),
			&(mx_dante_configuration->calib_energies_bins[1]) );

		if ( num_items != 2 ) {
			fprintf( stderr, " ... 2 items not seen" );
		}

	} else if ( strcmp( parameter_name, "calib_energies" ) == 0 ) {
		num_items = sscanf( parameter_string, "%lg;%lg",
			&(mx_dante_configuration->calib_energies[0]),
			&(mx_dante_configuration->calib_energies[1]) );

		if ( num_items != 2 ) {
			fprintf( stderr, " ... 2 items not seen" );
		}

	} else if ( strcmp( parameter_name, "calib_channels" ) == 0 ) {
		mx_dante_configuration->calib_channels =
					atol( parameter_string );

	} else if ( strcmp( parameter_name, "calib_equation" ) == 0 ) {
		mx_dante_configuration->calib_equation =
					atol( parameter_string );

	} else if ( strncmp( parameter_name, "/DPP", 4 ) == 0 ) {
		/* We throw away the XML </DPP> at the end
		 * of each MCA description in the XML file.
		 */
	} else if ( strncmp( parameter_name, "/Devices", 4 ) == 0 ) {
		/* We throw away the XML </Devices> at the end
		 * of the XML file.
		 */
	} else {
		if ( debug_parameters ) {
			fprintf( stderr, " ... not recognized" );
		}
	}

	if ( debug_parameters ) {
		fprintf( stderr, ".\n" );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

static mx_status_type
mxi_dante_copy_parameters_to_chain( MX_RECORD *dante_record,
			MX_DANTE_CONFIGURATION *chain_configuration )
{
	static const char fname[] = "mxi_dante_copy_parameters_to_chain()";

	MX_DANTE *dante = NULL;
	MX_RECORD *mca_record = NULL;
	MX_DANTE_MCA *dante_mca = NULL;
	char *chain_name = NULL;
	unsigned long i;

	if ( dante_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}
	if ( chain_configuration == (MX_DANTE_CONFIGURATION *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DANTE_CONFIGURATION pointer passed for DANTE "
		"record '%s' was NULL.", dante_record->name );
	}

	MX_DEBUG(-2,("%s: dante_record = '%s'.", fname, dante_record->name));

	chain_name = chain_configuration->chain_name;

	MX_DEBUG(-2,("%s: Copying DANTE configuration to chain '%s'.",
		fname, chain_name ));

	dante = (MX_DANTE *) dante_record->record_type_struct;

	if ( dante == (MX_DANTE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_DANTE pointer for DANTE record '%s' is NULL.",
			dante_record->name );
	}
	if ( dante->mca_record_array == (MX_RECORD **) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The mca_record_array pointer for DANTE record '%s' is NULL.",
			dante_record->name );
	}

	for ( i = 0; i < dante->num_mcas; i++ ) {
		mca_record = dante->mca_record_array[i];

		if ( mca_record == (MX_RECORD *) NULL ) {
			/* This MCA record slot is not in use, so skip on
			 * to the next slot.
			 */

			continue;
		}

		dante_mca = (MX_DANTE_MCA *) mca_record->record_type_struct;

		if ( dante_mca == (MX_DANTE_MCA *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_DANTE_MCA pointer for DANTE MCA record "
			"'%s' registered with DANTE record '%s' is NULL.",
		       		mca_record->name, dante_record->name );
		}

		/* Is this DANTE MCA part of the specific chain that we
		 * are looking for?
		 */

		if ( strcmp( chain_name, dante_mca->identifier ) == 0 ) {
			/* Yes, it is in the chain we are looking for. */

			MX_DEBUG(-2,("%s: Copying to MCA '%s'.",
				fname, mca_record->name ));

			/* The configuration does not contain any pointers,
			 * so it is safe to just copy it with memcpy().
			 */

			memcpy( &(dante_mca->mx_dante_configuration),
				chain_configuration,
				sizeof(dante_mca->mx_dante_configuration) );
		}
	}

	MX_DEBUG(-2,("%s complete.", fname));

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_dante_show_parameters( MX_RECORD *record )
{
	static const char fname[] = "mxi_dante_show_parameters()";

	MX_DANTE_MCA *dante_mca = (MX_DANTE_MCA *) record->record_type_struct;

	MX_DANTE_CONFIGURATION *mx_dante_configuration =
				&(dante_mca->mx_dante_configuration);

	struct configuration *configuration =
				&(mx_dante_configuration->configuration);

	MX_DEBUG(-2,("**** %s for MCA '%s'", fname, dante_mca->record->name));

	MX_DEBUG(-2,("  fast_filter_thr = %lu",
		(unsigned long) configuration->fast_filter_thr ));

	MX_DEBUG(-2,("  energy_filter_thr = %lu",
		(unsigned long) configuration->energy_filter_thr ));

	MX_DEBUG(-2,("  energy_baseline_thr = %lu",
		(unsigned long) configuration->energy_baseline_thr ));

	MX_DEBUG(-2,("  max_risetime = %g", configuration->max_risetime ));

	MX_DEBUG(-2,("  gain = %g", configuration->gain ));

	MX_DEBUG(-2,("  peaking_time = %lu",
		(unsigned long) configuration->peaking_time ));

	MX_DEBUG(-2,("  max_peaking_time = %lu",
		(unsigned long) configuration->max_peaking_time ));

	MX_DEBUG(-2,("  flat_top = %lu",
		(unsigned long) configuration->flat_top ));

	MX_DEBUG(-2,("  edge_peaking_time = %lu",
		(unsigned long) configuration->edge_peaking_time ));

	MX_DEBUG(-2,("  edge_flat_top = %lu",
		(unsigned long) configuration->edge_flat_top ));

	MX_DEBUG(-2,("  reset_recovery_time = %lu",
		(unsigned long) configuration->reset_recovery_time ));

	MX_DEBUG(-2,("  zero_peak_freq = %g", configuration->zero_peak_freq));

	MX_DEBUG(-2,("  baseline_samples = %lu",
		(unsigned long) configuration->baseline_samples ));

	if ( configuration->inverted_input ) {
		MX_DEBUG(-2,("  inverted_input = true" ));
	} else {
		MX_DEBUG(-2,("  inverted_input = false" ));
	}

	MX_DEBUG(-2,("  time_constant = %g", configuration->time_constant ));

	MX_DEBUG(-2,("  base_offset = %lu",
		(unsigned long) configuration->base_offset ));

	MX_DEBUG(-2,("  overflow_recovery = %lu",
		(unsigned long) configuration->overflow_recovery ));

	MX_DEBUG(-2,("  reset_threshold = %lu",
		(unsigned long) configuration->reset_threshold ));

	MX_DEBUG(-2,("  tail_coefficient = %g",
		configuration->tail_coefficient )); 

	MX_DEBUG(-2,("  other_param = %lu",
		(unsigned long) configuration->other_param ));

	MX_DEBUG(-2,("  timestamp_delay = %lu",
		dante_mca->mx_dante_configuration.timestamp_delay));

	MX_DEBUG(-2,("  baseline_offset = %lu",
	  (unsigned long) dante_mca->mx_dante_configuration.baseline_offset));

	MX_DEBUG(-2,("  offset = [ %lu, %lu ]",
		dante_mca->mx_dante_configuration.offset[0],
		dante_mca->mx_dante_configuration.offset[1] ));

	MX_DEBUG(-2,("  calib_energies_bins = [ %lu, %lu ]",
		dante_mca->mx_dante_configuration.calib_energies_bins[0],
		dante_mca->mx_dante_configuration.calib_energies_bins[1] ));

	MX_DEBUG(-2,("  calib_energies = [ %f, %f ]",
		dante_mca->mx_dante_configuration.calib_energies[0],
		dante_mca->mx_dante_configuration.calib_energies[1] ));

	MX_DEBUG(-2,("  calib_channels = %lu",
		dante_mca->mx_dante_configuration.calib_channels));

	MX_DEBUG(-2,("  calib_equation = %lu",
		dante_mca->mx_dante_configuration.calib_equation));

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_dante_show_parameters_for_chain( MX_RECORD *record )
{
	static const char fname[] = "mxi_dante_show_parameters_for_chain()";

	MX_DANTE *dante = NULL;
	MX_RECORD *mca_record = NULL;

#if MXI_DANTE_DEBUG_SHOW_PARAMETERS_FOR_CHAIN
	MX_DANTE_MCA *dante_mca = NULL;
#endif
	unsigned long i;
	mx_bool_type show_devices;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

#if MXI_DANTE_DEBUG_SHOW_PARAMETERS_FOR_CHAIN
	MX_DEBUG(-2,("%s invoked for '%s'.", fname, record->name ));
#endif

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

		mx_status = mxi_dante_show_parameters( mca_record );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

#if MXI_DANTE_DEBUG_SHOW_PARAMETERS_FOR_CHAIN
		dante_mca = (MX_DANTE_MCA *) mca_record->record_type_struct;

		MX_DEBUG(-2,("%s: dante_mca = %p", fname, dante_mca));
		MX_DEBUG(-2,("%s: spectrum_data = %p",
				fname, dante_mca->spectrum_data));
		MX_DEBUG(-2,("%s: spectrum_data[0] = %lu",
				fname, dante_mca->spectrum_data[0]));
#endif
	}

	return mx_status;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_dante_save_config_file( MX_RECORD *record )
{
	static const char fname[] = "mxi_dante_save_config_file()";

	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, record->name));

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_dante_load_config_file( MX_RECORD *dante_record )
{
	static const char fname[] = "mxi_dante_load_config_file()";

	MX_DANTE *dante = NULL;
	FILE *config_file = NULL;
	char *ptr = NULL;
	char *identifier_ptr = NULL;
	char *end_ptr = NULL;
	char *parameter_name = NULL;
	char *parameter_string = NULL;
	char *parameter_end = NULL;
	int saved_errno;
	unsigned long n;
	char buffer[200];

	MX_DANTE_CONFIGURATION chain_configuration;
	struct configuration new_configuration;

	char chain_name[MXU_DANTE_MAX_CHAIN_ID_LENGTH+1];
	mx_status_type mx_status;

	if ( dante_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

#if 1
	MX_DEBUG(-2,("%s invoked for '%s'.", fname, dante_record->name));
#endif

	dante = (MX_DANTE *) dante_record->record_type_struct;

	if ( dante == (MX_DANTE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_DANTE pointer for DANTE controller '%s' is NULL.",
			dante_record->name );
	}

	MX_DEBUG(-2,("%s: DANTE record '%s' will load parameters "
		"from config file '%s'.",
			fname, dante_record->name, dante->config_filename ));

	/* FIXME???: We are reading the XML by hand, in order to avoid having
	 * to find an XML library with an appropriate license to link to for
	 * this.  If we need to do much more XML processing in the future
	 * elsewhere in MX, we may want to revisit this choice here.
	 */

	/* mx_cfn_fopen( MX_CFN_CONFIG, ... ) tells MX that the default
	 * location of the Dante XML file is in the MX 'etc' directory.
	 */

	config_file = mx_cfn_fopen( MX_CFN_CONFIG,
				dante->config_filename, "r" );

	saved_errno = errno;
#if 1
	MX_DEBUG(-2,("%s: config_file = %p, errno = %d",
			fname, config_file, saved_errno ));
#endif

	if ( config_file == (FILE *) NULL ) {
		saved_errno = errno;

		switch( saved_errno ) {
		case EACCES:
			return mx_error( MXE_PERMISSION_DENIED, fname,
			"You do not have permission to read "
			"DANTE configuration file '%s' for record '%s'.",
				dante->config_filename, dante_record->name );
			break;
		case ENOENT:
			return mx_error( MXE_NOT_FOUND, fname,
			"DANTE configuration file '%s' cannot be found "
			"for record '%s'.",
				dante->config_filename, dante_record->name );
			break;
		default:
			return mx_error( MXE_FILE_IO_ERROR, fname,
			"Cannot open DANTE configuration file '%s' "
			"for record '%s'.  Errno = %d, error message = '%s'.",
				dante->config_filename, dante->record->name,
				saved_errno, strerror(saved_errno) );
			break;
		}
	}

	/* Loop over the master boards. */

	for ( n = 0; n < dante->num_master_devices; n++ ) {

	    /*  Work our way through the XML file for a single chain,
	     *  one line at a time.
	     */

	    chain_name[0] = '\0';

	    while (TRUE) {
	        /* Read one line from the XML file. */

	        ptr = mx_fgets( buffer, sizeof(buffer), config_file );

	        if ( ptr == (char *) NULL ) {

		    if ( feof(config_file) ) {
#if 1
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

#if 1
	        MX_DEBUG(-2,("%s: buffer = '%s'", fname, buffer));
#endif
		continue;	/*!!!!!!!!!*/

		/* We do not get to here. */

	        ptr = strstr( buffer, "<DPP Name" );

	        if ( ptr != (char *) NULL ) {
	    	/* This line appears to have a chain identifier in it. */

	    	identifier_ptr = strstr( ptr, "\"" );

	    	if ( identifier_ptr != NULL ) {
	    	    identifier_ptr++;

	    	    end_ptr = strstr( identifier_ptr, "\"" );

	    	    if ( end_ptr != NULL ) {
	    		*end_ptr = '\0';
	    	    }
	    	}

	    	if ( identifier_ptr != NULL ) {
	    	    strlcpy( chain_name, identifier_ptr, sizeof(chain_name) );

	    	    MX_DEBUG(-2,("%s: *** chain '%s' started.",
	    		fname, chain_name));

	    	    mx_status = mxi_dante_set_configuration_to_defaults(
	    			    &chain_configuration, chain_name );
	    	}
	        } else
	        if ( chain_name[0] != '\0' ) {
	    	/* If we are currently processing a chain's configuration
	    	 * parameters, see if this is one of the parameters.
	    	 */

	    	/* Look for the first '<' character on the line. */

	    	parameter_name = strchr( buffer, '<' );

	    	if ( parameter_name != (char *) NULL ) {
	    	    parameter_name++;

	    	    parameter_string = strchr( parameter_name, '>' );
	    	   
	    	    if ( parameter_string != (char *) NULL ) {
	    		*parameter_string = '\0';

	    		parameter_string++;

	    		parameter_end = strchr( parameter_string, '<' );

	    		if ( parameter_end != (char *) NULL ) {
	    		    *parameter_end = '\0';
	    		}

	    		if ( ( strcmp( parameter_name, "/DPP" ) != 0 )
	    		  && ( strcmp( parameter_name, "/Devices" ) != 0 ) )
	    		{
	    		    /* We have not yet reached the end of this chain's
	    		     * parameters, so we continue processing the
	    		     * parameters for the current chain.
	    		     */

	    		    mx_status = mxi_dante_set_parameter_from_string(
	    						&chain_configuration,
	    						parameter_name,
	    						parameter_string );

	    		    if ( mx_status.code != MXE_SUCCESS )
	    			    return mx_status;
	    		} else {
	    		    MX_DEBUG(-2,("%s: *** chain '%s' complete.",
	    			fname, chain_name));

	    		    /* We have reached the end of this chain's
	    		     * parameters, so we now look for all MCA
	    		     * records that are part of this chain and
	    		     * copy these parameters to them.
	    		     */

	    		    mx_status = mxi_dante_copy_parameters_to_chain(
	    		    	dante_record, &chain_configuration );

	    		    if ( mx_status.code != MXE_SUCCESS )
	    			    return mx_status;

	    		    mxi_dante_show_parameters_for_chain( dante_record );

	    		    /* We record the fact that we are finished
	    		     * with this particular chain's parameters.
	    		     */

	    		    chain_name[0] = '\0';
	    		}
	    	    }
	    	}
	        } else {
	    	/* Skip lines we are not interested in. */

	    	ptr = strstr( buffer, "</Devices" );

	    	if ( ptr == (char *) NULL ) {
	    		continue;
	    	}

	    	/* We are not in a set of chain parameters and we have not
	    	 * seen the character that marks the start of a chain,
	    	 * so print a warning message and continue on to the
	    	 * next line in the file.
	    	 */

	    	mx_warning( "A line was seen in the configuration file "
	    	"that does not seem to be part of a chain configuration, "
	    	"so we are skipping it.  line = '%s'", buffer );
	        }

	    } /* End of the while() loop over the XML for a single chain. */

	} /* End of the for() loop over chain masters. */

	/* FIXME: fclose() below crashes, so we ifdef it out for now.
	 * But we need to really understand why it is crashing, since
	 * by commenting it out here, we create a memory leak.  This
	 * might be related to the problem above with fopen().
	 */

#if 0
	fclose( config_file );
#endif

#if 1
	mxi_dante_close( dante_record );
	exit(0);
#endif

	return MX_SUCCESSFUL_RESULT;
}

