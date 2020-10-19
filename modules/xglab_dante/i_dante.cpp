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

#define MXI_DANTE_DEBUG_SHOW_CONFIG_FILE_BUFFERS	FALSE

#define MXI_DANTE_DEBUG_SECTIONS			FALSE

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
	MX_DANTE_CONFIGURATION *dante_configuration = NULL;
	unsigned long i, j, attempt;

	unsigned long major, minor, update, extra;
	int num_items;

	unsigned long max_init_delay_ms;
	unsigned long max_attempts;

	bool dante_status;
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

#if 0
	if ( dante->dante_flags & MXF_DANTE_SHOW_DEVICES ) {
		show_devices = true;
	} else {
		show_devices = false;
	}
#endif

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

		dante->master[i].dante = dante;

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

			if ( dante_status != false ) {
				break;	/* Exit the for(attempt) loop. */
			}

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

			mx_msleep(1000);
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
		}

		MX_DEBUG(-2,("%s: All boards found for '%s'.",
		fname, dante->master[i].chain_id ));

		MX_DEBUG(-2,("%s: &(dante->master[%lu]) = %p",
		fname, i, &(dante->master[i]) ));

		dante->master[i].num_boards = num_boards;

		/* Allocate and initialize the MX_DANTE_CONFIGURATION array
		 * for this MX_DANTE_CHAIN struct.
		 */

		dante_configuration = (MX_DANTE_CONFIGURATION *)
			calloc( num_boards, sizeof(MX_DANTE_CONFIGURATION) );

		if ( dante_configuration == NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Could not allocate a %d element array of "
			"MX_DANTE_CONFIGURATION structures for chain_id '%s' "
			"of Dante record '%s'.",
				num_boards,
				dante->master[i].chain_id,
				dante->record->name );
		}

		dante->master[i].dante_configuration = dante_configuration;

		for ( j = 0; j < num_boards; j++ ) {
			dante_configuration[j].chain = &(dante->master[i]);
		}

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

	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	MX_DEBUG(-2,("%s invoked for '%s'.", fname, record->name ));

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
		MX_DANTE_CONFIGURATION *mx_dante_configuration )
{
	static const char fname[] = "mxi_dante_set_configuration_to_defaults()";

	if ( mx_dante_configuration == (MX_DANTE_CONFIGURATION *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DANTE_CONFIGURATION pointer passed was NULL." );
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

#if 0
static mx_status_type
mxi_dante_copy_parameters_to_chain( MX_RECORD *dante_record,
			MX_DANTE_CONFIGURATION *chain_configuration )
{
	static const char fname[] = "mxi_dante_copy_parameters_to_chain()";

	MX_DANTE *dante = NULL;
	MX_RECORD *mca_record = NULL;
	MX_DANTE_MCA *dante_mca = NULL;
	char *chain_id = NULL;
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

	chain_id = chain_configuration->chain->chain_id;

	MX_DEBUG(-2,("%s: Copying DANTE configuration to chain '%s'.",
		fname, chain_id ));

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

		if ( strcmp( chain_id, dante_mca->identifier ) == 0 ) {
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
#endif

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxi_dante_show_parameters( MX_RECORD *record )
{
	static const char fname[] = "mxi_dante_show_parameters()";

	MX_DANTE_MCA *dante_mca = NULL;
	MX_DANTE_CONFIGURATION *mx_dante_configuration = NULL;
	struct configuration *configuration = NULL;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	dante_mca = (MX_DANTE_MCA *) record->record_type_struct;

	if ( dante_mca == (MX_DANTE_MCA *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_DANTE_MCA pointer for record '%s' is NULL.",
			record->name );
	}

	mx_dante_configuration = dante_mca->mx_dante_configuration;

	if ( mx_dante_configuration == (MX_DANTE_CONFIGURATION *) NULL ) {
		return mx_error( MXE_INITIALIZATION_ERROR, fname,
		"The MX_DANTE_CONFIGURATION pointer has not yet been "
		"initialized for Dante MCA record '%s'.  "
		"Perhaps the startup configuration of MX has not yet finished?",
			record->name );
	}

	configuration = &(mx_dante_configuration->configuration);

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
		mx_dante_configuration->timestamp_delay));

	MX_DEBUG(-2,("  baseline_offset = %lu",
		mx_dante_configuration->baseline_offset));

	MX_DEBUG(-2,("  offset = [ %lu, %lu ]",
		mx_dante_configuration->offset[0],
		mx_dante_configuration->offset[1] ));

	MX_DEBUG(-2,("  calib_energies_bins = [ %lu, %lu ]",
		mx_dante_configuration->calib_energies_bins[0],
		mx_dante_configuration->calib_energies_bins[1] ));

	MX_DEBUG(-2,("  calib_energies = [ %f, %f ]",
		mx_dante_configuration->calib_energies[0],
		mx_dante_configuration->calib_energies[1] ));

	MX_DEBUG(-2,("  calib_channels = %lu",
		mx_dante_configuration->calib_channels));

	MX_DEBUG(-2,("  calib_equation = %lu",
		mx_dante_configuration->calib_equation));

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
mxi_dante_set_data_available_flag_for_chain( MX_RECORD *original_mca_record,
						mx_bool_type new_flag_value )
{
	static const char fname[] =
		"mxi_dante_set_data_available_flag_for_chain()";

	MX_DANTE_MCA *original_dante_mca = NULL;
	MX_DANTE_CONFIGURATION *original_dante_configuration = NULL;
	MX_DANTE_CHAIN *dante_chain = NULL;
	unsigned long i;
	MX_RECORD *current_mca_record = NULL;
	MX_MCA *current_mca = NULL;

	if ( original_mca_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

#if 1
	MX_DEBUG(-2,("%s invoked for MCA '%s'.",
			fname, original_mca_record->name));
#endif

	/* Find the Dante chain that this MCA is part of. */

	original_dante_mca = (MX_DANTE_MCA *)
				original_mca_record->record_type_struct;

	if ( original_dante_mca == (MX_DANTE_MCA *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_DANTE_MCA pointer for Dante MCA '%s' is NULL.",
			original_mca_record->name );
	}

	original_dante_configuration =
			original_dante_mca->mx_dante_configuration;

	if ( original_dante_configuration == (MX_DANTE_CONFIGURATION *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_DANTE_CONFIGURATION pointer for "
		"Dante MCA '%s' is NULL.", original_mca_record->name );
	}

	dante_chain = original_dante_configuration->chain;

	if ( dante_chain == (MX_DANTE_CHAIN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_DANTE_CHAIN pointer for Dante MCA '%s' is NULL.",
			original_mca_record->name );
	}

	/* Walk through the MX_DANTE_CHAIN setting 'new_data_available' flags
	 * along the way.
	 */

	for ( i = 0; i < dante_chain->num_boards; i++ ) {
		current_mca_record =
			dante_chain->dante_configuration[i].mca_record;

		if ( current_mca_record == (MX_RECORD *) NULL ) {
			mx_warning( "The MCA record pointer for chain item %lu "
			"in the MX_DANTE_CHAIN for original MCA '%s' is NULL.",
				i, original_mca_record->name );

			continue;	/* Go to the next MCA record. */
		}

		current_mca = (MX_MCA *)
				current_mca_record->record_class_struct;

		if ( current_mca == (MX_MCA *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_MCA pointer for Dante MCA record '%s' "
			"is NULL.", current_mca_record->name );
		}

		current_mca->new_data_available = new_flag_value;

		MX_DEBUG(-2,("%s: 'new_data_available' set to %d for MCA '%s'.",
			fname, (int) current_mca->new_data_available,
			current_mca_record->name ));
	}

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

/*-------------------------------------------------------------------------*/

/* Some special values for DPP section numbers. */

#define MX_DPP_IN_EXAMPLE_SECTION	(-1)
#define MX_DPP_IN_COMMON_SECTION	(-2)
#define MX_DPP_NOT_IN_SECTION		(-9999)

MX_EXPORT mx_status_type
mxi_dante_load_config_file( MX_RECORD *dante_record )
{
	static const char fname[] = "mxi_dante_load_config_file()";

	MX_DANTE *dante = NULL;
	MX_DANTE_CHAIN *chain_master = NULL;
	MX_DANTE_CONFIGURATION *dante_configuration = NULL;
	MX_RECORD *mca_record = NULL;
	MX_DANTE_MCA *dante_mca = NULL;
	FILE *config_file = NULL;
	unsigned long i;
	char *section_name_ptr = NULL;
	char *section_name_end_ptr = NULL;
	char *master_name_ptr = NULL;
	char *channel_name_ptr = NULL;
	long dpp_section = MX_DPP_NOT_IN_SECTION;
	long channel_number = LONG_MIN;
	int num_items = -1;
	char *ptr = NULL;
	char *parameter_name = NULL;
	char *parameter_string = NULL;
	char *parameter_end = NULL;
	int saved_errno;
	char buffer[200];

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

	/* Work our way through the XML file one line at a time. */

	/* NOTE: It is arguable that we may want to use an external XML
	 * parser library here.  For now we have chosen to parse the XML by
	 * hand, but this code is near the upper limit of what we would
	 * want to do by hand.
	 *
	 * As of October 2020, the most likely alternatives are:
	 *
	 * libxml2 - an MIT licensed DOM-style parser which covers nearly
	 *           all of our platforms.
	 *
	 * expat - an MIT licensed SAX-style parser which may cover fewer
	 *         of our platforms than libxml2.
	 *
	 * Please note that any external XML library used must NOT change
	 * the license for MX as a whole from the MIT style MX uses now.
	 */

	/* Loop over the master boards. */

	for ( i = 0; i < dante->num_master_devices; i++ ) {

	    dpp_section = MX_DPP_NOT_IN_SECTION;

	    chain_master = &(dante->master[i]);

	    dante_configuration = NULL;

	    if ( i == 0 ) {
		    MX_DEBUG(-2,("%s: &(dante->master[0]) = %p",
			fname, chain_master));
	    }

	    /* Verify that the chain_id and num_boards fields have already
	     * been set in the 'chain_master' structure.
	     */

	    if ( chain_master->chain_id[0] == '\0' ) {
		    return mx_error( MXE_SOFTWARE_CONFIGURATION_ERROR, fname,
		    "Master %lu of Dante record '%s' has not had its "
		    "chain id set already.  This should not happen.",
		    	i, dante->record->name );
	    }

	    if ( chain_master->num_boards == 0 ) {
		    return mx_error( MXE_HARDWARE_CONFIGURATION_ERROR, fname,
		    "Master %lu '%s' of Dante record '%s' says that it has "
		    "0 boards.  This should not be able to happen, since "
		    "the master board itself should be included in the "
		    "count of boards.",
		    	i, chain_master->chain_id, dante->record->name );
	    }

	    while (1) {
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

#if MXI_DANTE_DEBUG_SHOW_CONFIG_FILE_BUFFERS
	        MX_DEBUG(-2,("%s: buffer = '%s'", fname, buffer));
#endif
		/*------------------------------------------------------*/

		/* Is this the 'xml version' string at the beginning
		 * of the file?
		 */

		ptr = strstr( buffer, "<\?xml version" );

		if ( ptr != (char *) NULL ) {
			/* We have found the xml version line. */

			continue; /* Go back to the top of the while(1) loop. */
#if 0
			mxi_dante_close(dante_record);
			exit(0);
#endif
		}

		/*------------------------------------------------------*/

		/* Are we at the beginning of a Device? */

		ptr = strstr( buffer, "<Devices" );

		if ( ptr != (char *) NULL ) {
			/* The Name in the <Devices line does not contain
			 * information that we are going to use, so we
			 * just skip over it.
			 */

			continue; /* Go back to the top of the while(1) loop.*/
		}

		/*------------------------------------------------------*/

		/* Are we at the end of a Device? */

		ptr = strstr( buffer, "</Devices" );

		if ( ptr != (char *) NULL ) {
			chain_master = NULL;

			/* Break out of the while(1) loop to go on to
			 * the next master board.
			 */

			break; /* Leave the while(1) loop. */
		}

		/*------------------------------------------------------*/

		/* Are we at the end of a DPP section? */

		ptr = strstr( buffer, "</DPP" );

		if ( ptr != (char *) NULL ) {
#if MXI_DANTE_DEBUG_SECTIONS
			MX_DEBUG(-2,("%s: *** Leaving section %ld ***",
					fname, dpp_section ));
#endif

			dante_configuration = NULL;

			dpp_section = MX_DPP_NOT_IN_SECTION;

			continue; /* Go back to the top of the while(1) loop.*/
		}

		/*------------------------------------------------------*/

		/* Are we at the beginning of a DPP section? */

		ptr = strstr( buffer, "<DPP" );

		if ( ptr != (char *) NULL ) {
		    dante_configuration = NULL;

		    if ( strcmp( ptr, "<DPP Name=\"Example SN\">" ) == 0 ) {
			dpp_section = MX_DPP_IN_EXAMPLE_SECTION;
		    } else
		    if ( strcmp( ptr, "<DPP_COMMON_CONFIG>" ) == 0 ) {
			dpp_section = MX_DPP_IN_COMMON_SECTION;
		    } else {
			/* Find the section name. */

			section_name_ptr = strchr( ptr, '\"' );

			if ( section_name_ptr == (char *) NULL ) {
			    mx_warning(
			    "DPP section name start not found in '%s'", ptr );

			    /* Go back to the top of the while(1) loop. */
			    continue;
			}

			section_name_ptr++;	/* Skip over the '"' char. */

			section_name_end_ptr = strchr( section_name_ptr, '\"' );

			if ( section_name_end_ptr == (char *) NULL ) {
			    mx_warning(
			    "DPP section name end not found in '%s'", ptr );

			    /* Go back to the top of the while(1) loop. */
			    continue;
			}

			/* Null terminate the section name. */

			*section_name_end_ptr = '\0';

#if MXI_DANTE_DEBUG_SECTIONS
			MX_DEBUG(-2,("%s: section_name_ptr = '%s'",
					fname, section_name_ptr ));
#endif

			/* Parse the section name.  The part before the
			 * '-' character should be the name of the 
			 * master device for this chain.  The part after
			 * the '_' should be 'Chx' where x is the number
			 * of the section.
			 */

			master_name_ptr = section_name_ptr;

			channel_name_ptr = strchr( master_name_ptr, '_' );

			if ( channel_name_ptr == (char *) NULL ) {
			    mx_warning(
			    "Channel name not found in section name '%s'",
				section_name_ptr );

			    /* Go back to the top of the while(1) loop. */
			    continue;
			}

			*channel_name_ptr = '\0';
			channel_name_ptr++;

#if MXI_DANTE_DEBUG_SECTIONS
			MX_DEBUG(-2,("%s: section '%s', channel '%s'",
			fname, master_name_ptr, channel_name_ptr ));
#endif

			/* Get the channel number. */

			num_items = sscanf( channel_name_ptr, "Ch%ld",
					&channel_number );

			if ( num_items != 1 ) {
			    mx_warning( "Did not find the channel number "
				"in '%s.", channel_name_ptr );

			    /* Go back to the top of the while(1) loop. */
			    continue;
			}

			/* Check that the chain id matches the master id. */

			if ( strcmp(chain_master->chain_id,
					master_name_ptr) == 0 )
			{
#if MXI_DANTE_DEBUG_SECTIONS
			    MX_DEBUG(-2,("%s === Verified in chain '%s'",
				fname, chain_master->chain_id ));
#endif
			} else {
			    mx_warning( "We are supposed to be in chain '%s', "
			    "but the section header says we are in chain '%s'.",
			    	chain_master->chain_id, master_name_ptr );
			}

			/* All seems to be well. */

			dpp_section = channel_number - 1;

			/* Setup the link of the MX_DANTE_CONFIGURATION
			 * pointer to the matching MX_DANTE_MCA structure.
			 */

			dante_configuration =
			    &(chain_master->dante_configuration[dpp_section]);

			if ( dante_configuration == NULL ) {
			    return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			    "The MX_DANTE_CONFIGURATION pointer for DPP "
			    "section %lu for Dante record '%s' is NULL.",
			    	dpp_section, dante->record->name );
			}

			/* We need to find the mca_record_array element
			 * that matches this DPP section.
			 */

			mca_record = dante->mca_record_array[ dpp_section ];

			if ( mca_record == (MX_RECORD *) NULL ) {
			    return mx_error( MXE_INITIALIZATION_ERROR, fname,
			    "We are attempting to make use of the MCA record "
			    "in element %lu of the mca_record_array for "
			    "Dante record '%s'.  But this element has not yet "
			    "been initialized.",
				dpp_section, dante->record->name );
			}

			dante_mca = (MX_DANTE_MCA *)
					mca_record->record_type_struct;

			if ( dante_mca == (MX_DANTE_MCA *) NULL ) {
			    return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
				"The MX_DANTE_MCA pointer for "
				"Dante MCA '%s' is NULL.",
					mca_record->name );
			}

			if ( dante_mca->mx_dante_configuration != NULL ) {
			    return mx_error( MXE_INITIALIZATION_ERROR, fname,
				"We are attempting to store the "
				"MX_DANTE_CONFIGURATION pointer for "
				"DPP section %lu to Dante MCA '%s'.  "
				"But that pointer seems to already "
				"be in use.", dpp_section, mca_record->name );
			}

			dante_mca->mx_dante_configuration = dante_configuration;
		    }

#if MXI_DANTE_DEBUG_SECTIONS
		    MX_DEBUG(-2,("%s: *** Entering section %ld ***",
					fname, dpp_section ));
#endif

		    continue; /* Go back to the top of the while(1) loop.*/
		}

		/* Everything from here on in the XML parsing loop
		 * should be part of one of the identified DPP sections.
		 * If a valid section number has not been set for here,
		 * then the structure of the XML config file does not
		 * match what we expect.
		 */

		if ( dpp_section <= MX_DPP_NOT_IN_SECTION ) {
			return mx_error( MXE_NOT_VALID_FOR_CURRENT_STATE, fname,
			"We are in the part of the XML parsing of "
			"configuration file '%s' for Dante record '%s' "
			"where we should be parsing individual parameters, "
			"but a valid section number has not been set.  "
			"The section number seen is (%ld).",
				"xyzzy", dante->record->name, dpp_section );
		}

		/* We are not interested in the parameters found in
		 * the fake 'Example SN' DPP, so we skip over them.
		 */

		if ( dpp_section == MX_DPP_IN_EXAMPLE_SECTION ) {

			continue; /* Go back to the top of the while(1) loop.*/
		}

		/* Parameters from the DPP_COMMON_CONFIG section 
		 * are handled separately from the regular DPP
		 * parameters.
		 */

		if ( dpp_section == MX_DPP_IN_COMMON_SECTION ) {
			mx_warning("Skipping common: '%s'", buffer);

			continue; /* Go back to the top of the while(1) loop.*/
		}

		/* We have reached the section that handles ordinary
		 * DPP parameters.
		 */

		if ( dante_configuration == (MX_DANTE_CONFIGURATION *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"An attempt is being made to set the value of "
			"a parameter for DPP section %lu of "
			"Dante record '%s', but the dante_configuration "
			"pointer is NULL, which makes it impossible.",
				dpp_section, dante->record->name );
		}

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

			/* Set the value of the parameter we have found. */

			mx_status = mxi_dante_set_parameter_from_string(
	    						dante_configuration,
	    						parameter_name,
	    						parameter_string );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

	    	    }
	    	}

	    } /* End of the while() loop over the XML for a single chain. */

	} /* End of the for() loop over chain masters. */

	mx_fclose( config_file );

	return MX_SUCCESSFUL_RESULT;
}

