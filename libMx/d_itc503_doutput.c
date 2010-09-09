/*
 * Name:    d_itc503_doutput.c
 *
 * Purpose: MX digital output driver for the Oxford Instruments ITC503
 *          temperature controller.
 *
 *          Please note that this driver only writes status values.  It does
 *          not attempt to change the temperature settings.
 *
 * Author:  William Lavender and Henry Bellamy
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2003-2006, 2008-2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define ITC503_DOUTPUT_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_digital_output.h"
#include "i_isobus.h"
#include "i_itc503.h"
#include "d_itc503_doutput.h"

MX_RECORD_FUNCTION_LIST mxd_itc503_doutput_record_function_list = {
	NULL,
	mxd_itc503_doutput_create_record_structures,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_itc503_doutput_resynchronize
};

MX_DIGITAL_OUTPUT_FUNCTION_LIST
	mxd_itc503_doutput_digital_output_function_list =
{
	mxd_itc503_doutput_read,
	mxd_itc503_doutput_write
};

MX_RECORD_FIELD_DEFAULTS mxd_itc503_doutput_field_default[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DIGITAL_OUTPUT_STANDARD_FIELDS,
	MXD_ITC503_DOUTPUT_STANDARD_FIELDS
};

long mxd_itc503_doutput_num_record_fields
		= sizeof( mxd_itc503_doutput_field_default )
		/ sizeof( mxd_itc503_doutput_field_default[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_itc503_doutput_rfield_def_ptr
			= &mxd_itc503_doutput_field_default[0];

/* ===== */

static mx_status_type
mxd_itc503_doutput_get_pointers( MX_DIGITAL_OUTPUT *doutput,
				MX_ITC503_DOUTPUT **itc503_doutput,
				MX_ITC503 **itc503,
				MX_ISOBUS **isobus,
				const char *calling_fname )
{
	static const char fname[] = "mxd_itc503_doutput_get_pointers()";

	MX_ITC503_DOUTPUT *itc503_doutput_ptr;
	MX_RECORD *itc503_record, *isobus_record;
	MX_ITC503 *itc503_ptr;

	if ( doutput == (MX_DIGITAL_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DIGITAL_OUTPUT pointer passed was NULL." );
	}

	if ( doutput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
    "The MX_RECORD pointer for the MX_DIGITAL_OUTPUT pointer passed was NULL." );
	}

	itc503_doutput_ptr = (MX_ITC503_DOUTPUT *)
				doutput->record->record_type_struct;

	if ( itc503_doutput_ptr == (MX_ITC503_DOUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_ITC503_DOUTPUT pointer for digital output '%s' is NULL",
			doutput->record->name );
	}

	if ( itc503_doutput != (MX_ITC503_DOUTPUT **) NULL ) {
		*itc503_doutput = itc503_doutput_ptr;
	}

	itc503_record = itc503_doutput_ptr->itc503_record;

	if ( itc503_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"itc503_record pointer for digital output '%s' is NULL.",
			doutput->record->name );
	}

	switch( itc503_record->mx_type ) {
	case MXI_CTRL_ITC503:
	case MXI_CTRL_CRYOJET:
		break;
	default:
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"itc503_record '%s' for ITC503 control record '%s' "
		"is not an 'itc503' or a 'cryojet' record.  Instead, "
		"it is of type '%s'.",
			itc503_record->name, doutput->record->name,
			mx_get_driver_name( itc503_record ) );
		break;
	}

	itc503_ptr = (MX_ITC503 *) itc503_record->record_type_struct;

	if ( itc503_ptr == (MX_ITC503 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_ITC503 pointer for ITC503 controller '%s' "
		"used by ITC503 status record '%s' is NULL.",
			itc503_record->name,
			doutput->record->name );
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
				itc503_record->name );
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
mxd_itc503_doutput_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_itc503_doutput_create_record_structures()";

        MX_DIGITAL_OUTPUT *digital_output;
        MX_ITC503_DOUTPUT *itc503_doutput;

        /* Allocate memory for the necessary structures. */

        digital_output = (MX_DIGITAL_OUTPUT *) malloc(sizeof(MX_DIGITAL_OUTPUT));

        if ( digital_output == (MX_DIGITAL_OUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_DIGITAL_OUTPUT structure." );
        }

        itc503_doutput = (MX_ITC503_DOUTPUT *)
				malloc( sizeof(MX_ITC503_DOUTPUT) );

        if ( itc503_doutput == (MX_ITC503_DOUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
               "Can't allocate memory for MX_ITC503_DOUTPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = digital_output;
        record->record_type_struct = itc503_doutput;
        record->class_specific_function_list
			= &mxd_itc503_doutput_digital_output_function_list;

        digital_output->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_itc503_doutput_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxd_itc503_doutput_resynchronize()";

	MX_ITC503_DOUTPUT *itc503_doutput;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for record '%s'.",
		fname, record->name));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	itc503_doutput =
		(MX_ITC503_DOUTPUT *) record->record_type_struct;

	if ( itc503_doutput == (MX_ITC503_DOUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_ITC503_DOUTPUT pointer for digital output '%s' is NULL",
			record->name );
	}

	mx_status = mx_resynchronize_record( itc503_doutput->itc503_record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_itc503_doutput_read( MX_DIGITAL_OUTPUT *doutput )
{
	static const char fname[] = "mxd_itc503_doutput_read()";

	MX_ITC503_DOUTPUT *itc503_doutput = NULL;
	MX_ITC503 *itc503 = NULL;
	MX_ISOBUS *isobus = NULL;
	char command[10];
	char response[80];
	char parameter_type;
	int parse_failure;
	mx_status_type mx_status;

	mx_status = mxd_itc503_doutput_get_pointers( doutput,
				&itc503_doutput, &itc503, &isobus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	parameter_type = itc503_doutput->parameter_type;

	if ( islower( (int) parameter_type ) ) {
		parameter_type = toupper( (int) parameter_type );
	}

	parse_failure = FALSE;

	switch( parameter_type ) {
	case 'A':	/* Auto/manual status */

		strlcpy( command, "X", sizeof(command) );

		mx_status = mxi_isobus_command( isobus,
					itc503->isobus_address,
					command, response, sizeof(response),
					itc503->maximum_retries,
					ITC503_DOUTPUT_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( ( response[0] != 'X' )
		  || ( response[2] != 'A' ) )
		{
			parse_failure = TRUE;
		} else {
			doutput->value =
				mx_hex_char_to_unsigned_long( response[3] );
		}
		break;

	case 'C':	/* Local/remote/lock status */

		strlcpy( command, "X", sizeof(command) );

		mx_status = mxi_isobus_command( isobus,
					itc503->isobus_address,
					command, response, sizeof(response),
					itc503->maximum_retries,
					ITC503_DOUTPUT_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( ( response[0] != 'X' )
		  || ( response[4] != 'C' ) )
		{
			parse_failure = TRUE;
		} else {
			doutput->value =
				mx_hex_char_to_unsigned_long( response[5] );
		}
		break;

	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Unsupported %s control parameter type '%c' for record '%s'.",
			itc503->label,
			itc503_doutput->parameter_type,
			doutput->record->name );
	}

	if ( parse_failure ) {
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
		"Could not parse the response by %s controller '%s' to "
		"the command '%s'.  Response = '%s'",
			itc503->label, itc503->record->name, command, response);
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_itc503_doutput_write( MX_DIGITAL_OUTPUT *doutput )
{
	static const char fname[] = "mxd_itc503_doutput_write()";

	MX_ITC503_DOUTPUT *itc503_doutput = NULL;
	MX_ITC503 *itc503 = NULL;
	MX_ISOBUS *isobus = NULL;
	char command[80];
	char response[80];
	char parameter_type;
	mx_status_type mx_status;

	mx_status = mxd_itc503_doutput_get_pointers( doutput,
				&itc503_doutput, &itc503, &isobus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	parameter_type = itc503_doutput->parameter_type;

	if ( islower( (int)parameter_type ) ) {
		parameter_type = toupper( (int) parameter_type );
	}

	switch( parameter_type ) {
	case 'A':	/* Auto/manual command */
		/* For the Cryojet it is necessary to follow the A0 with
		 * a O0 to set the heater voltage to zero. To enable  
		 * automatic temperature control an A1 will suffice.
		 */

		if ( ( doutput->value != 0 )
		  && ( doutput->value != 1 ) )
		{
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The 'A' auto/manual control value passed (%ld) for record '%s' "
	"is not one of the allowed values, 0 or 1.",
				doutput->value,
				doutput->record->name );
		}

		snprintf( command, sizeof(command), "A%ld", doutput->value );

		mx_status = mxi_isobus_command( isobus,
					itc503->isobus_address,
					command, response, sizeof(response),
					itc503->maximum_retries,
					ITC503_DOUTPUT_DEBUG );

		if ( ( itc503->record->mx_type == MXI_CTRL_CRYOJET ) 
		  && (doutput->value == 0 ) )
		{
			snprintf( command, sizeof(command), "O0" );
			
			mx_status = mxi_isobus_command( isobus,
					itc503->isobus_address,
					command, response, sizeof(response),
					itc503->maximum_retries,
					ITC503_DOUTPUT_DEBUG );
		}
		break;

	case 'C':	/* Local/remote/lock command */

		if ( doutput->value > 3 ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The 'C' local/remote/lock control value passed (%ld) for record '%s' "
	"is not in the allowed range of values from 0 to 3.",
				doutput->value,
				doutput->record->name );
		}

		snprintf( command, sizeof(command), "C%ld", doutput->value );

		mx_status = mxi_isobus_command( isobus,
					itc503->isobus_address,
					command, response, sizeof(response),
					itc503->maximum_retries,
					ITC503_DOUTPUT_DEBUG );
		break;

	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Unsupported %s control parameter type '%c' for record '%s'.",
			itc503->label,
			itc503_doutput->parameter_type,
			doutput->record->name );
	}
	return mx_status;
}

