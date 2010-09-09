/*
 * Name:    d_itc503_aoutput.c
 *
 * Purpose: MX analog output driver for the Oxford Instruments ITC503
 *          temperature controller and the Cryojet cryocooler.
 *
 *          ITC503: Please note that this driver only writes status values.
 *          It does not attempt to change the temperature settings.
 *
 *	    Cryojet: Can control gas flow rates, temperature settings etc.
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

#define ITC503_AOUTPUT_DEBUG	FALSE
 
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_analog_output.h"
#include "i_isobus.h"
#include "i_itc503.h"
#include "d_itc503_aoutput.h"

MX_RECORD_FUNCTION_LIST mxd_itc503_aoutput_record_function_list = {
	NULL,
	mxd_itc503_aoutput_create_record_structures,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_itc503_aoutput_resynchronize
};

MX_ANALOG_OUTPUT_FUNCTION_LIST
	mxd_itc503_aoutput_analog_output_function_list =
{
	mxd_itc503_aoutput_read,
	mxd_itc503_aoutput_write
};

MX_RECORD_FIELD_DEFAULTS mxd_itc503_aoutput_field_default[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_DOUBLE_ANALOG_OUTPUT_STANDARD_FIELDS,
	MX_ANALOG_OUTPUT_STANDARD_FIELDS,
	MXD_ITC503_AOUTPUT_STANDARD_FIELDS
};

long mxd_itc503_aoutput_num_record_fields
		= sizeof( mxd_itc503_aoutput_field_default )
		/ sizeof( mxd_itc503_aoutput_field_default[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_itc503_aoutput_rfield_def_ptr
			= &mxd_itc503_aoutput_field_default[0];

/* ===== */

static mx_status_type
mxd_itc503_aoutput_get_pointers( MX_ANALOG_OUTPUT *aoutput,
				MX_ITC503_AOUTPUT **itc503_aoutput,
				MX_ITC503 **itc503,
				MX_ISOBUS **isobus,
				const char *calling_fname )
{
	static const char fname[] = "mxd_itc503_aoutput_get_pointers()";

	MX_ITC503_AOUTPUT *itc503_aoutput_ptr;
	MX_RECORD *itc503_record, *isobus_record;
	MX_ITC503 *itc503_ptr;

	if ( aoutput == (MX_ANALOG_OUTPUT *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_ANALOG_OUTPUT pointer passed was NULL." );
	}

	if ( aoutput->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
    "The MX_RECORD pointer for the MX_ANALOG_OUTPUT pointer passed was NULL." );
	}

	itc503_aoutput_ptr = (MX_ITC503_AOUTPUT *)
				aoutput->record->record_type_struct;

	if ( itc503_aoutput_ptr == (MX_ITC503_AOUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_ITC503_AOUTPUT pointer for analog output '%s' is NULL",
			aoutput->record->name );
	}

	if ( itc503_aoutput != (MX_ITC503_AOUTPUT **) NULL ) {
		*itc503_aoutput = itc503_aoutput_ptr;
	}

	itc503_record = itc503_aoutput_ptr->itc503_record;

	if ( itc503_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"itc503_record pointer for analog output '%s' is NULL.",
			aoutput->record->name );
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
			itc503_record->name, aoutput->record->name,
			mx_get_driver_name( itc503_record ) );
		break;
	}

	itc503_ptr = (MX_ITC503 *) itc503_record->record_type_struct;

	if ( itc503_ptr == (MX_ITC503 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_ITC503 pointer for ITC503 controller '%s' "
		"used by ITC503 status record '%s' is NULL.",
			itc503_record->name,
			aoutput->record->name );
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
mxd_itc503_aoutput_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
		"mxd_itc503_aoutput_create_record_structures()";

        MX_ANALOG_OUTPUT *analog_output;
        MX_ITC503_AOUTPUT *itc503_aoutput;

        /* Allocate memory for the necessary structures. */

        analog_output = (MX_ANALOG_OUTPUT *) malloc(sizeof(MX_ANALOG_OUTPUT));

        if ( analog_output == (MX_ANALOG_OUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
                "Can't allocate memory for MX_ANALOG_OUTPUT structure." );
        }

        itc503_aoutput = (MX_ITC503_AOUTPUT *)
				malloc( sizeof(MX_ITC503_AOUTPUT) );

        if ( itc503_aoutput == (MX_ITC503_AOUTPUT *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
               "Can't allocate memory for MX_ITC503_AOUTPUT structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = analog_output;
        record->record_type_struct = itc503_aoutput;
        record->class_specific_function_list
			= &mxd_itc503_aoutput_analog_output_function_list;

        analog_output->record = record;

	/* Raw analog output values are stored as doubles. */

	analog_output->subclass = MXT_AOU_DOUBLE;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_itc503_aoutput_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxd_itc503_aoutput_resynchronize()";

	MX_ITC503_AOUTPUT *itc503_aoutput;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for record '%s'.",
		fname, record->name));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	itc503_aoutput =
		(MX_ITC503_AOUTPUT *) record->record_type_struct;

	if ( itc503_aoutput == (MX_ITC503_AOUTPUT *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_ITC503_AOUTPUT pointer for analog output '%s' is NULL",
			record->name );
	}

	mx_status = mx_resynchronize_record( itc503_aoutput->itc503_record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_itc503_aoutput_read( MX_ANALOG_OUTPUT *aoutput )
{
	static const char fname[] = "mxd_itc503_aoutput_read()";

	MX_ITC503_AOUTPUT *itc503_aoutput = NULL;
	MX_ITC503 *itc503 = NULL;
	MX_ISOBUS *isobus = NULL;
	char command[10];
	char response[80];
	char buffer[5];
	char parameter_type;
	double double_value;
	int num_items, parse_failure;
	mx_status_type mx_status;

	mx_status = mxd_itc503_aoutput_get_pointers( aoutput,
				&itc503_aoutput, &itc503, &isobus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	parameter_type = itc503_aoutput->parameter_type;

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
					ITC503_AOUTPUT_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( ( response[0] != 'X' )
		  || ( response[2] != 'A' ) )
		{
			parse_failure = TRUE;
		} else {
			buffer[0] = response[3];
			buffer[1] = '\0';

			double_value = atof( buffer );
		}
		break;

	case 'C':	/* Local/remote/lock status */

		strlcpy( command, "X", sizeof(command) );

		mx_status = mxi_isobus_command( isobus,
					itc503->isobus_address,
					command, response, sizeof(response),
					itc503->maximum_retries,
					ITC503_AOUTPUT_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( ( response[0] != 'X' )
		  || ( response[4] != 'C' ) )
		{
			parse_failure = TRUE;
		} else {
			buffer[0] = response[5];
			buffer[1] = '\0';

			double_value = atof( buffer );
		}
		break;

	case 'D':	/* Derivative action time */

		strlcpy( command, "R10", sizeof(command) );

		mx_status = mxi_isobus_command( isobus,
					itc503->isobus_address,
					command, response, sizeof(response),
					itc503->maximum_retries,
					ITC503_AOUTPUT_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "R%lg", &double_value );

		if ( num_items != 1 ) {
			parse_failure = TRUE;
		}
		break;

	case 'G':	/* Gas flow - only for ITC503 */

		if ( itc503->record->mx_type != MXI_CTRL_ITC503 ) {
			return mx_error( MXE_UNSUPPORTED, fname,
			"The %s controller used by record '%s' is not "
			"supported for the 'G' command.  Only the ITC503 "
			"is supported for that command.",
				itc503->label,
				itc503->record->name );
		}

		strlcpy( command, "R7", sizeof(command) );

		mx_status = mxi_isobus_command( isobus,
					itc503->isobus_address,
					command, response, sizeof(response),
					itc503->maximum_retries,
					ITC503_AOUTPUT_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "R%lg", &double_value );

		if ( num_items != 1 ) {
			parse_failure = TRUE;
		}
		break;

	case 'I':	/* Integral action time */

		strlcpy( command, "R9", sizeof(command) );

		mx_status = mxi_isobus_command( isobus,
					itc503->isobus_address,
					command, response, sizeof(response),
					itc503->maximum_retries,
					ITC503_AOUTPUT_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "R%lg", &double_value );

		if ( num_items != 1 ) {
			parse_failure = TRUE;
		}
		break;

	case 'J':	/* Shield flow - only for Cryojet */

		if ( itc503->record->mx_type != MXI_CTRL_CRYOJET ) {
			return mx_error( MXE_UNSUPPORTED, fname,
			"The %s controller used by record '%s' is not "
			"supported for the 'J' command.  Only the Cryojet "
			"is supported for that command.",
				itc503->label,
				itc503->record->name );
		}

		strlcpy( command, "R18", sizeof(command) );

		mx_status = mxi_isobus_command( isobus,
					itc503->isobus_address,
					command, response, sizeof(response),
					itc503->maximum_retries,
					ITC503_AOUTPUT_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "R%lg", &double_value );

		if ( num_items != 1 ) {
			parse_failure = TRUE;
		}
		break;

	case 'K':	/* Sample flow - only for Cryojet */

		if ( itc503->record->mx_type != MXI_CTRL_CRYOJET ) {
			return mx_error( MXE_UNSUPPORTED, fname,
			"The %s controller used by record '%s' is not "
			"supported for the 'K' command.  Only the Cryojet "
			"is supported for that command.",
				itc503->label,
				itc503->record->name );
		}

		strlcpy( command, "R19", sizeof(command) );

		mx_status = mxi_isobus_command( isobus,
					itc503->isobus_address,
					command, response, sizeof(response),
					itc503->maximum_retries,
					ITC503_AOUTPUT_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "R%lg", &double_value );

		if ( num_items != 1 ) {
			parse_failure = TRUE;
		}
		break;

	case 'O':	/* Heater output (volts) */

		strlcpy( command, "R5", sizeof(command) );

		mx_status = mxi_isobus_command( isobus,
					itc503->isobus_address,
					command, response, sizeof(response),
					itc503->maximum_retries,
					ITC503_AOUTPUT_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "R%lg", &double_value );

		if ( num_items != 1 ) {
			parse_failure = TRUE;
		}
		break;

	case 'P':	/* Proportional band */

		strlcpy( command, "R8", sizeof(command) );

		mx_status = mxi_isobus_command( isobus,
					itc503->isobus_address,
					command, response, sizeof(response),
					itc503->maximum_retries,
					ITC503_AOUTPUT_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "R%lg", &double_value );

		if ( num_items != 1 ) {
			parse_failure = TRUE;
		}
		break;

	case 'T':	/* Temperature set point (kelvin) */

		strlcpy( command, "R0", sizeof(command) );

		mx_status = mxi_isobus_command( isobus,
					itc503->isobus_address,
					command, response, sizeof(response),
					itc503->maximum_retries,
					ITC503_AOUTPUT_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "R%lg", &double_value );

		if ( num_items != 1 ) {
			parse_failure = TRUE;
		}
		break;

	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Unsupported %s control parameter type '%c' for record '%s'.",
			itc503->label,
			itc503_aoutput->parameter_type,
			aoutput->record->name );
	}

	if ( parse_failure ) {
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
		"Could not parse the response by %s controller '%s' to "
		"the command '%s'.  Response = '%s'",
			itc503->label, itc503->record->name, command, response);
	}

	aoutput->raw_value.double_value = double_value;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_itc503_aoutput_write( MX_ANALOG_OUTPUT *aoutput )
{
	static const char fname[] = "mxd_itc503_aoutput_write()";

	MX_ITC503_AOUTPUT *itc503_aoutput = NULL;
	MX_ITC503 *itc503 = NULL;
	MX_ISOBUS *isobus = NULL;
	char command[80];
	char response[80];
	long long_parameter_value;
	double double_parameter_value;
	char parameter_type;
	mx_status_type mx_status;

	mx_status = mxd_itc503_aoutput_get_pointers( aoutput,
				&itc503_aoutput, &itc503, &isobus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	parameter_type = itc503_aoutput->parameter_type;

	if ( islower( (int)parameter_type ) ) {
		parameter_type = toupper( (int) parameter_type );
	}

	switch( parameter_type ) {
	case 'A':	/* Auto/manual command */

		/* For the Cryojet it is necessary to follow the A0 with
		 * a O0 to set the heater voltage to zero. To enable  
		 * automatic temperature control an A1 will suffice
		 */

		long_parameter_value = 
			mx_round( aoutput->raw_value.double_value );

		if ( ( long_parameter_value != 0 )
		  && ( long_parameter_value != 1 ) )
		{
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The 'A' auto/manual control value passed (%g) for record '%s' "
	"is not one of the allowed values, 0 or 1.",
				aoutput->raw_value.double_value,
				aoutput->record->name );
		}

		snprintf( command, sizeof(command),
				"A%1ld", long_parameter_value );

		mx_status = mxi_isobus_command( isobus,
					itc503->isobus_address,
					command, response, sizeof(response),
					itc503->maximum_retries,
					ITC503_AOUTPUT_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( ( itc503->record->mx_type == MXI_CTRL_CRYOJET ) 
		  && ( long_parameter_value == 0 ) )
		{
			snprintf( command, sizeof(command), "O0" );
			
			mx_status = mxi_isobus_command( isobus,
					itc503->isobus_address,
					"00", response, sizeof(response),
					itc503->maximum_retries,
					ITC503_AOUTPUT_DEBUG );
		}
		break;

	case 'C':	/* Local/remote/lock command */

		long_parameter_value = 
			mx_round( aoutput->raw_value.double_value );

		if ( ( long_parameter_value < 0 )
		  || ( long_parameter_value > 3 ) )
		{
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The 'C' local/remote/lock control value passed (%g) for record '%s' "
	"is not in the allowed range of values from 0 to 3.",
				aoutput->raw_value.double_value,
				aoutput->record->name );
		}

		snprintf( command, sizeof(command),
				"C%1ld", long_parameter_value );

		mx_status = mxi_isobus_command( isobus,
					itc503->isobus_address,
					command, response, sizeof(response),
					itc503->maximum_retries,
					ITC503_AOUTPUT_DEBUG );
		break;

	case 'D':	/* Derivative action time */

		double_parameter_value = aoutput->raw_value.double_value;

		if ( ( double_parameter_value < 0 )
		  || ( double_parameter_value > 273.0 ) )
		{
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The 'D' derivative action time passed (%g) for record '%s' "
	"is not in the allowed range of values from 0.0 to 273.0.",
			double_parameter_value, aoutput->record->name );
		}

		snprintf( command, sizeof(command),
				"D%.1f", double_parameter_value );

		mx_status = mxi_isobus_command( isobus,
					itc503->isobus_address,
					command, response, sizeof(response),
					itc503->maximum_retries,
					ITC503_AOUTPUT_DEBUG );
		break;

	case 'G':	/* Gas flow command - only for ITC503 */

		if ( itc503->record->mx_type != MXI_CTRL_ITC503 ) {
			return mx_error( MXE_UNSUPPORTED, fname,
			"The %s controller used by record '%s' is not "
			"supported for the 'G' command.  Only the ITC503 "
			"is supported for that command.",
				itc503->label,
				itc503->record->name );
		}

		long_parameter_value =
			mx_round( 10.0 * aoutput->raw_value.double_value );

		if ( ( long_parameter_value < 0 )
		  || ( long_parameter_value > 999 ) )
		{
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The 'G' gas flow control value passed (%g) for record '%s' "
	"is not in the allowed range of values from 0.0 to 99.9.",
				aoutput->raw_value.double_value,
				aoutput->record->name );
		}

	
		snprintf( command, sizeof(command),
				"G%03ld", long_parameter_value );

		mx_status = mxi_isobus_command( isobus,
					itc503->isobus_address,
					command, response, sizeof(response),
					itc503->maximum_retries,
					ITC503_AOUTPUT_DEBUG );
		break;

	case 'I':	/* Integral action time */

		double_parameter_value = aoutput->raw_value.double_value;

		if ( ( double_parameter_value < 0 )
		  || ( double_parameter_value > 140.0 ) )
		{
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The 'I' integral action time passed (%g) for record '%s' "
	"is not in the allowed range of values from 0.0 to 140.0.",
			double_parameter_value, aoutput->record->name );
		}

		snprintf( command, sizeof(command),
				"I%.1f", double_parameter_value );

		mx_status = mxi_isobus_command( isobus,
					itc503->isobus_address,
					command, response, sizeof(response),
					itc503->maximum_retries,
					ITC503_AOUTPUT_DEBUG );
		break;

	case 'J':	/* Shield flow - only for Cryojet */
		
		if ( itc503->record->mx_type != MXI_CTRL_CRYOJET ) {
			return mx_error( MXE_UNSUPPORTED, fname,
			"The %s controller used by record '%s' is not "
			"supported for the 'J' command.  Only the Cryojet "
			"is supported for that command.",
				itc503->label,
				itc503->record->name );
		}

		double_parameter_value = aoutput->raw_value.double_value;

		if ( ( double_parameter_value < 0 )
		  || ( double_parameter_value > 10.0 ) )
		{
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The 'J' shield flow control value passed (%g) for record '%s' "
	"is not in the allowed range of values from 0.0 to 10.0.",
			double_parameter_value, aoutput->record->name );
		}

		snprintf( command, sizeof(command),
				"J%.1f", double_parameter_value );

		mx_status = mxi_isobus_command( isobus,
					itc503->isobus_address,
					command, response, sizeof(response),
					itc503->maximum_retries,
					ITC503_AOUTPUT_DEBUG );
		break;

	case 'K':	/* Sample flow - only for Cryojet */
		
		if ( itc503->record->mx_type != MXI_CTRL_CRYOJET ) {
			return mx_error( MXE_UNSUPPORTED, fname,
			"The %s controller used by record '%s' is not "
			"supported for the 'K' command.  Only the Cryojet "
			"is supported for that command.",
				itc503->label,
				itc503->record->name );
		}

		double_parameter_value = aoutput->raw_value.double_value;

		if ( ( double_parameter_value < 0 )
		  || ( double_parameter_value > 10.0 ) )
		{
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The 'K' sample flow control value passed (%g) for record '%s' "
	"is not in the allowed range of values from 0.0 to 10.0.",
			double_parameter_value, aoutput->record->name );
		}

		snprintf( command, sizeof(command),
				"K%.1f", double_parameter_value );

		mx_status = mxi_isobus_command( isobus,
					itc503->isobus_address,
					command, response, sizeof(response),
					itc503->maximum_retries,
					ITC503_AOUTPUT_DEBUG );
		break;

	case 'O':	/* Heater output command */

		double_parameter_value = aoutput->raw_value.double_value;

		if ( ( double_parameter_value < 0 )
		  || ( double_parameter_value > 48.0 ) )
		{
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The 'O' heater output control value passed (%g) for record '%s' "
	"is not in the allowed range of values from 0.0 to 48.0.",
			double_parameter_value, aoutput->record->name );
		}

		snprintf( command, sizeof(command),
				"O%.1f", double_parameter_value );


		mx_status = mxi_isobus_command( isobus,
					itc503->isobus_address,
					command, response, sizeof(response),
					itc503->maximum_retries,
					ITC503_AOUTPUT_DEBUG );
		break;

	case 'P':	/* Proportional band */

		double_parameter_value = aoutput->raw_value.double_value;

		if ( ( double_parameter_value < 0 )
		  || ( double_parameter_value > 100.0 ) )
		{
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The 'P' proportional band control value passed (%g) for record '%s' "
	"is not in the allowed range of values from 0.0 to 100.0.",
			double_parameter_value, aoutput->record->name );
		}

		snprintf( command, sizeof(command),
				"P%.2f", double_parameter_value );

		mx_status = mxi_isobus_command( isobus,
					itc503->isobus_address,
					command, response, sizeof(response),
					itc503->maximum_retries,
					ITC503_AOUTPUT_DEBUG );
		break;

	case 'T':	/* Temperature setpoint */
		
		double_parameter_value = aoutput->raw_value.double_value;

		if ( ( double_parameter_value < 0 )
		  || ( double_parameter_value > 320.00 ) )
		{
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The 'T' temperature setpoint value passed (%g) for record '%s' "
	"is not in the allowed range of values from 0.0 to 320.00.",
				aoutput->raw_value.double_value,
				aoutput->record->name );
		}

		snprintf( command, sizeof(command),
				"T%.2f", double_parameter_value );

		mx_status = mxi_isobus_command( isobus,
					itc503->isobus_address,
					command, response, sizeof(response),
					itc503->maximum_retries,
					ITC503_AOUTPUT_DEBUG );
		break;

	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Unsupported %s control parameter type '%c' for record '%s'.",
			itc503->label,
			itc503_aoutput->parameter_type,
			aoutput->record->name );
	}
	return mx_status;
}

