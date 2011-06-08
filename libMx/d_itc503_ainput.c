/*
 * Name:    d_itc503_ainput.c
 *
 * Purpose: MX analog input driver for Oxford Instruments ITC503 and
 *          Cryojet temperature controllers.
 *
 *          Please note that this driver only reads status values.  It does
 *          not attempt to change the temperature settings.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2003-2006, 2008, 2010-2011 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define ITC503_AINPUT_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_motor.h"
#include "mx_analog_input.h"
#include "i_isobus.h"
#include "i_itc503.h"
#include "d_itc503_ainput.h"

MX_RECORD_FUNCTION_LIST mxd_itc503_ainput_record_function_list = {
	NULL,
	mxd_itc503_ainput_create_record_structures,
	mx_analog_input_finish_record_initialization,
	NULL,
	NULL,
	mxd_itc503_ainput_open,
	NULL,
	NULL,
	mxd_itc503_ainput_resynchronize
};

MX_ANALOG_INPUT_FUNCTION_LIST
	mxd_itc503_ainput_analog_input_function_list =
{
	mxd_itc503_ainput_read
};

MX_RECORD_FIELD_DEFAULTS mxd_itc503_ainput_field_default[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DOUBLE_ANALOG_INPUT_STANDARD_FIELDS,
	MX_ANALOG_INPUT_STANDARD_FIELDS,
	MXD_ITC503_AINPUT_STANDARD_FIELDS
};

long mxd_itc503_ainput_num_record_fields
		= sizeof( mxd_itc503_ainput_field_default )
		/ sizeof( mxd_itc503_ainput_field_default[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_itc503_ainput_rfield_def_ptr
			= &mxd_itc503_ainput_field_default[0];

/* ===== */

static mx_status_type
mxd_itc503_ainput_get_pointers( MX_ANALOG_INPUT *ainput,
				MX_ITC503_AINPUT **itc503_ainput,
				MX_ITC503 **itc503,
				MX_ISOBUS **isobus,
				const char *calling_fname )
{
	static const char fname[] = "mxd_itc503_ainput_get_pointers()";

	MX_ITC503_AINPUT *itc503_ainput_ptr;
	MX_RECORD *controller_record, *isobus_record;
	MX_ITC503 *itc503_ptr;

	if ( ainput == (MX_ANALOG_INPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_ANALOG_INPUT pointer passed was NULL." );
	}

	if ( ainput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
    "The MX_RECORD pointer for the MX_ANALOG_INPUT pointer passed was NULL." );
	}

	itc503_ainput_ptr = (MX_ITC503_AINPUT *)
				ainput->record->record_type_struct;

	if ( itc503_ainput_ptr == (MX_ITC503_AINPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_ITC503_AINPUT pointer for analog input '%s' is NULL",
			ainput->record->name );
	}

	if ( itc503_ainput != (MX_ITC503_AINPUT **) NULL ) {
		*itc503_ainput = itc503_ainput_ptr;
	}

	controller_record = itc503_ainput_ptr->controller_record;

	if ( controller_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"controller_record pointer for analog input '%s' is NULL.",
			ainput->record->name );
	}

	switch( controller_record->mx_type ) {
	case MXI_CTRL_ITC503:
	case MXI_CTRL_CRYOJET:
		break;
	default:
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"controller_record '%s' for ITC503 control record '%s' "
		"is not an 'itc503' or 'cryojet' record.  Instead, "
		"it is of type '%s'.",
			controller_record->name, ainput->record->name,
			mx_get_driver_name( controller_record ) );
		break;
	}

	itc503_ptr = (MX_ITC503 *) controller_record->record_type_struct;

	if ( itc503_ptr == (MX_ITC503 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_ITC503 pointer for ITC503 controller '%s' "
		"used by ITC503 status record '%s' is NULL.",
			controller_record->name,
			ainput->record->name );
	}

	if ( itc503 != (MX_ITC503 **) NULL ) {
		*itc503 = itc503_ptr;
	}

	if ( isobus != (MX_ISOBUS **) NULL ) {
		isobus_record = itc503_ptr->isobus_record;

		if ( isobus_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The isobus_record pointer for %s "
			"controller record '%s' is NULL.",
				itc503_ptr->label,
				controller_record->name );
		}

		*isobus = isobus_record->record_type_struct;

		if ( (*isobus) == (MX_ISOBUS *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_ISOBUS pointer for ISOBUS record '%s' "
			"is NULL.", isobus_record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ===== */

MX_EXPORT mx_status_type
mxd_itc503_ainput_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_itc503_ainput_create_record_structures()";

        MX_ANALOG_INPUT *analog_input;
        MX_ITC503_AINPUT *itc503_ainput;

        /* Allocate memory for the necessary structures. */

        analog_input = (MX_ANALOG_INPUT *) malloc(sizeof(MX_ANALOG_INPUT));

        if ( analog_input == (MX_ANALOG_INPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_ANALOG_INPUT structure." );
        }

        itc503_ainput = (MX_ITC503_AINPUT *)
				malloc( sizeof(MX_ITC503_AINPUT) );

        if ( itc503_ainput == (MX_ITC503_AINPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
               "Can't allocate memory for MX_ITC503_AINPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = analog_input;
        record->record_type_struct = itc503_ainput;
        record->class_specific_function_list
			= &mxd_itc503_ainput_analog_input_function_list;

        analog_input->record = record;

	/* Raw analog input values are stored as doubles. */

	analog_input->subclass = MXT_AIN_DOUBLE;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_itc503_ainput_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_itc503_ainput_open()";

	MX_ANALOG_INPUT *ainput;
	MX_ITC503_AINPUT *itc503_ainput = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	ainput = record->record_class_struct;

	mx_status = mxd_itc503_ainput_get_pointers( ainput,
				&itc503_ainput, NULL, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Check to see if the requested parameter type is supported
	 * for this device.
	 */

	switch( record->mx_type ) {
	case MXT_AIN_ITC503:
		if ( (itc503_ainput->parameter_type < 0)
		  || (itc503_ainput->parameter_type > 13) )
		{
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The requested parameter type %ld for ITC503 "
			"analog input record '%s' is not valid.  "
			"The allowed values are from 1 to 13.",
				itc503_ainput->parameter_type,
				record->name );
		}
		break;

	case MXT_AIN_CRYOJET:
		switch( itc503_ainput->parameter_type ) {
		case 0:
		case 1:

		case 4:
		case 5:
		case 6:

		case 8:
		case 9:
		case 10:
		case 11:

		case 18:
		case 19:
			break;
		default:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The requested parameter type %ld for Cryojet "
			"analog input record '%s' is not valid.  "
			"The allowed values are 0, 1, 4, 5, 6, 8, 9, "
			"10, 11, 18, and 19.",
				itc503_ainput->parameter_type,
				record->name );
			break;
		}
		break;

	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Record '%s' is not supported by this driver.",
			record->name );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_itc503_ainput_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxd_itc503_ainput_resynchronize()";

	MX_ITC503_AINPUT *itc503_ainput;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for record '%s'.",
		fname, record->name));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	itc503_ainput =
		(MX_ITC503_AINPUT *) record->record_type_struct;

	if ( itc503_ainput == (MX_ITC503_AINPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_ITC503_AINPUT pointer for analog input '%s' is NULL",
			record->name );
	}

	mx_status = mx_resynchronize_record( itc503_ainput->controller_record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_itc503_ainput_read( MX_ANALOG_INPUT *ainput )
{
	static const char fname[] = "mxd_itc503_ainput_read()";

	MX_ITC503_AINPUT *itc503_ainput = NULL;
	MX_ITC503 *itc503 = NULL;
	MX_ISOBUS *isobus = NULL;
	char command[80];
	char response[80];
	int num_items;
	double double_value;
	mx_status_type mx_status;

	mx_status = mxd_itc503_ainput_get_pointers( ainput,
				&itc503_ainput, &itc503, &isobus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( ( itc503_ainput->parameter_type < 0 )
	  || ( itc503_ainput->parameter_type > 19 ) )
	{
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Illegal parameter type %ld requested for %s analog input record '%s'."
	"  Allowed parameter types are from 0 to 19.  Read the description of "
	"the 'R' command in the ITC503 manual for more information.",
			itc503_ainput->parameter_type,
			itc503->label,
			ainput->record->name );
	}

	/* The requested parameter type is used directly to construct
	 * an ITC503 'R' command.
	 */

	snprintf( command, sizeof(command),
			"R%ld", itc503_ainput->parameter_type );

	/* Send the READ command to the controller. */

	mx_status = mxi_isobus_command( isobus,
					itc503->isobus_address,
					command, response, sizeof(response),
					itc503->maximum_retries,
					ITC503_AINPUT_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "R%lg", &double_value );

	if ( num_items != 1 ) {
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
		"Could not parse the response by %s controller '%s' to "
		"the READ command '%s'.  Response = '%s'",
			itc503->label, itc503->record->name, command, response);
	}

	ainput->raw_value.double_value = double_value;

	return MX_SUCCESSFUL_RESULT;
}

