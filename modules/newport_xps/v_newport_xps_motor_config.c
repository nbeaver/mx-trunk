/*
 * Name:    v_newport_xps_motor_config.c
 *
 * Purpose: Support for custom Newport XPS motor configuration.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXV_NEWPORT_XPS_MOTOR_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_motor.h"
#include "mx_variable.h"
#include "i_newport_xps.h"
#include "d_newport_xps.h"
#include "v_newport_xps_motor_config.h"

#include "XPS_C8_drivers.h"    /* Vendor include file. */
#include "XPS_C8_errors.h"     /* Vendor include file. */

MX_RECORD_FUNCTION_LIST mxv_newport_xps_motor_config_record_function_list = {
	mx_variable_initialize_driver,
	mxv_newport_xps_motor_config_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxv_newport_xps_motor_config_open
};

MX_VARIABLE_FUNCTION_LIST mxv_newport_xps_motor_config_variable_function_list =
{
	mxv_newport_xps_motor_config_send_variable
};

/*---*/

MX_RECORD_FIELD_DEFAULTS mxv_newport_xps_motor_config_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXV_NEWPORT_XPS_MOTOR_CONFIG_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_STRING_VARIABLE_STANDARD_FIELDS
};

long mxv_newport_xps_motor_config_num_record_fields
	= sizeof( mxv_newport_xps_motor_config_field_defaults )
	/ sizeof( mxv_newport_xps_motor_config_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_newport_xps_motor_config_rfield_def_ptr
		= &mxv_newport_xps_motor_config_field_defaults[0];

/*---*/

static mx_status_type
mxv_newport_xps_motor_config_get_pointers( MX_VARIABLE *variable,
			MX_NEWPORT_XPS_MOTOR_CONFIG **newport_xps_motor_config,
			MX_NEWPORT_XPS_MOTOR **newport_xps_motor,
			MX_NEWPORT_XPS **newport_xps,
			const char *calling_fname )
{
	static const char fname[] =
		"mxv_newport_xps_motor_config_get_pointers()";

	MX_NEWPORT_XPS_MOTOR_CONFIG *newport_xps_motor_config_ptr;
	MX_RECORD *newport_xps_motor_record;
	MX_NEWPORT_XPS_MOTOR *newport_xps_motor_ptr;
	MX_RECORD *newport_xps_record;

	if ( variable == (MX_VARIABLE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_VARIABLE pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( variable->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_RECORD pointer for MX_VARIABLE pointer passed by '%s' was NULL.",
			calling_fname );
	}

	newport_xps_motor_config_ptr = (MX_NEWPORT_XPS_MOTOR_CONFIG *)
				variable->record->record_type_struct;

	if ( newport_xps_motor_config_ptr
		== (MX_NEWPORT_XPS_MOTOR_CONFIG *) NULL )
	{
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_NEWPORT_XPS_MOTOR_CONFIG pointer "
		"for record '%s' is NULL.",
			variable->record->name );
	}

	if (newport_xps_motor_config != (MX_NEWPORT_XPS_MOTOR_CONFIG **) NULL) {
		*newport_xps_motor_config = newport_xps_motor_config_ptr;
	}

	newport_xps_motor_record =
		newport_xps_motor_config_ptr->newport_xps_motor_record;

	if ( newport_xps_motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"newport_xps_motor_record pointer for "
		"'newport_xps_motor_config' variable record '%s' "
		"passed by '%s' is NULL.",
			variable->record->name, calling_fname );
	}

	newport_xps_motor_ptr = (MX_NEWPORT_XPS_MOTOR *)
				newport_xps_motor_record->record_type_struct;

	if ( newport_xps_motor_ptr == (MX_NEWPORT_XPS_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_NEWPORT_XPS_MOTOR pointer for motor '%s' "
		"used by variable '%s' is NULL.",
			newport_xps_motor_record->name,
			variable->record->name );
	}

	if ( newport_xps_motor != (MX_NEWPORT_XPS_MOTOR **) NULL ) {
		*newport_xps_motor = newport_xps_motor_ptr;
	}

	if ( newport_xps != (MX_NEWPORT_XPS **) NULL ) {
		newport_xps_record = newport_xps_motor_ptr->newport_xps_record;

		if ( newport_xps_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The newport_xps_record pointer for motor '%s' "
			"used by variable '%s' is NULL.",
				newport_xps_motor_record->name,
				variable->record->name );
		}

		*newport_xps = (MX_NEWPORT_XPS *)
					newport_xps_record->record_type_struct;

		if ( (*newport_xps) == (MX_NEWPORT_XPS *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_NEWPORT_XPS pointer for 'newport_xps' "
			"record '%s' used by motor '%s' and variable '%s' "
			"is NULL.",
				newport_xps_record->name,
				newport_xps_motor_record->name,
				variable->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/********************************************************************/

MX_EXPORT mx_status_type
mxv_newport_xps_motor_config_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxv_newport_xps_motor_config_create_record_structures()";

	MX_VARIABLE *variable_struct;
	MX_NEWPORT_XPS_MOTOR_CONFIG *newport_xps_motor_config;

	/* Allocate memory for the necessary structures. */

	variable_struct = (MX_VARIABLE *) malloc( sizeof(MX_VARIABLE) );

	if ( variable_struct == (MX_VARIABLE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_VARIABLE structure." );
	}

	newport_xps_motor_config = (MX_NEWPORT_XPS_MOTOR_CONFIG *)
				malloc( sizeof(MX_NEWPORT_XPS_MOTOR_CONFIG) );

	if ( newport_xps_motor_config == (MX_NEWPORT_XPS_MOTOR_CONFIG *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	  "Cannot allocate memory for MX_NEWPORT_XPS_MOTOR_CONFIG structure." );
	}

	/* Now set up the necessary pointers. */

	variable_struct->record = record;

	record->record_superclass_struct = variable_struct;
	record->record_class_struct = NULL;
	record->record_type_struct = newport_xps_motor_config;

	record->superclass_specific_function_list =
			&mxv_newport_xps_motor_config_variable_function_list;
	record->class_specific_function_list = NULL;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxv_newport_xps_motor_config_open( MX_RECORD *record )
{
	static const char fname[] = "mxv_newport_xps_motor_config_open()";

	MX_VARIABLE *variable;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	variable = (MX_VARIABLE *) record->record_superclass_struct;

	mx_status = mxv_newport_xps_motor_config_get_pointers( variable,
						NULL, NULL, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxv_newport_xps_motor_config_send_variable( variable );

	return mx_status;
}

MX_EXPORT mx_status_type
mxv_newport_xps_motor_config_send_variable( MX_VARIABLE *variable )
{
	static const char fname[] =
		"mxv_newport_xps_motor_config_send_variable()";

	MX_NEWPORT_XPS_MOTOR_CONFIG *newport_xps_motor_config;
	MX_NEWPORT_XPS_MOTOR *newport_xps_motor;
	MX_NEWPORT_XPS *newport_xps;
	MX_RECORD_FIELD *value_field;
	char *value_string;
	char *value_string_copy;
	char *config_name;
	int argc;
	char **argv;
	int xps_status;
	mx_status_type mx_status;

	mx_status = mxv_newport_xps_motor_config_get_pointers( variable,
						&newport_xps_motor_config,
						&newport_xps_motor,
						&newport_xps,
						fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	value_field = mx_get_record_field( variable->record, "value" );

	if ( value_field == (MX_RECORD_FIELD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The variable '%s' does not have a 'value' field!  "
		"This should not be able to happen.",
			variable->record->name );
	}

	value_string = mx_get_field_value_pointer( value_field );

	if ( value_string == (char *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The field value pointer for the '%s' field "
		"of record '%s' is NULL.",
			value_string, variable->record->name );
	}

	value_string_copy = strdup( value_string );

	if ( value_string_copy == (char *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"The attempt by record '%s' to make a copy "
		"of string '%s' ran out of memory.",
			variable->record->name,
			value_string );
	}

	mx_string_split( value_string_copy, " ,", &argc, &argv );

	if ( argc < 1 ) {
		mx_free( argv ); mx_free( value_string_copy );

		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The variable value for variable record '%s' is empty.",
			variable->record->name );
	}

	config_name = newport_xps_motor_config->newport_xps_motor_config_name;

	if ( mx_strcasecmp( "aquadb_always_enable", config_name ) == 0 ) {
		mx_bool_type always_enable;

		always_enable = mx_get_true_or_false( argv[0] );

		MX_DEBUG(-2,("%s: variable '%s', always_enable = %d",
			fname, variable->record->name, (int) always_enable));

		if ( always_enable ) {
			xps_status =
			    PositionerPositionCompareAquadBAlwaysEnable(
					newport_xps->socket_id,
					newport_xps_motor->positioner_name );

			if ( xps_status != SUCCESS ) {
				mx_free( argv ); mx_free( value_string_copy );

				return mxi_newport_xps_error(
					newport_xps->socket_id,
				"PositionerPositionCompareAquadBAlwaysEnable()",
					xps_status );
			}
		} else {
			xps_status = PositionerPositionCompareDisable(
					newport_xps->socket_id,
					newport_xps_motor->positioner_name );

			if ( xps_status != SUCCESS ) {
				mx_free( argv ); mx_free( value_string_copy );

				return mxi_newport_xps_error(
					newport_xps->socket_id,
					"PositionerPositionCompareDisable()",
					xps_status );
			}
		}
	} else
	if ( mx_strcasecmp( "aquadb_windowed", config_name ) == 0 ) {
		double window_low, window_high;

		if ( argc < 2 ) {
			mx_free( argv ); mx_free( value_string_copy );

			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"aquadb_windowed mode for variable '%s' has fewer "
			"than 2 arguments in its value string '%s'.",
				variable->record->name, value_string );
		}

		window_low  = atof( argv[0] );
		window_high = atof( argv[1] );

		MX_DEBUG(-2,("%s: '%s' aquadb_windowed = %f, %f",
		    fname, variable->record->name, window_low, window_high));

		/* Set the window. */

		xps_status = PositionerPositionCompareAquadBWindowedSet(
					newport_xps->socket_id,
					newport_xps_motor->positioner_name,
					window_low, window_high );

		if ( xps_status != SUCCESS ) {
			mx_free( argv ); mx_free( value_string_copy );

			return mxi_newport_xps_error(
				newport_xps->socket_id,
				"PositionerPositionCompareAquadBWindowedSet()",
				xps_status );
		}

		/* Turn position compare on. */

		xps_status = PositionerPositionCompareEnable(
					newport_xps->socket_id,
					newport_xps_motor->positioner_name );

		if ( xps_status != SUCCESS ) {
			mx_free( argv ); mx_free( value_string_copy );

			return mxi_newport_xps_error(
				newport_xps->socket_id,
				"PositionerPositionCompareEnable()",
				xps_status );
		}

#if 1
		{
		  int enabled;

		  /* Verify that the window was set correctly. */

		  window_low = window_high = 0;

		  xps_status = PositionerPositionCompareAquadBWindowedGet(
					newport_xps->socket_id,
					newport_xps_motor->positioner_name,
					&window_low, &window_high,
					&enabled );

		  if ( xps_status != SUCCESS ) {
			mx_free( argv ); mx_free( value_string_copy );

			return mxi_newport_xps_error(
				newport_xps->socket_id,
				"PositionerPositionCompareAquadBWindowedSet()",
				xps_status );
		  }

		  MX_DEBUG(-2,("%s: '%s' aquadb_windowed values read = %f, %f",
		    fname, variable->record->name, window_low, window_high));
		}
#endif

	} else {
		mx_free( argv ); mx_free( value_string_copy );

		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Variable '%s' has a config name '%s' that is not recognized.",
			variable->record->name, config_name );
	}

	return mx_status;
}

