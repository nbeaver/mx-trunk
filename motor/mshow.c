/*
 * Name:    mshow.c
 *
 * Purpose: Motor show parameters functions.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999-2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "mx_inttypes.h"
#include "mx_analog_input.h"
#include "mx_analog_output.h"
#include "mx_digital_input.h"
#include "mx_digital_output.h"
#include "mx_scaler.h"
#include "mx_timer.h"
#include "mx_plot.h"
#include "mx_array.h"
#include "mx_version.h"
#include "motor.h"

#ifndef max
#define max(a,b) ((a) >= (b) ? (a) : (b))
#endif

int
motor_showall_fn( int argc, char *argv[] )
{
	int status;
	size_t length;
	char usage[] = "Usage:  showall record 'name'\n";

	if ( argc != 4 ) {
		fprintf( output, usage );
		return FAILURE;
	}

	length = strlen(argv[2]);

	if ( length == 0 )
		length = 1;

	if ( strncmp( "record", argv[2], length ) != 0 ) {
		fprintf(output,"show: Unrecognized option '%s'\n\n", argv[2]);
		fprintf(output, usage);
		return FAILURE;
	}

	status = motor_show_record(
			MXR_ANY, MXC_ANY, MXT_ANY, argv[2], argv[3], TRUE );

	return status;
}

int
motor_show_fn( int argc, char *argv[] )
{
	static const char cname[] = "show";

	char record_type_phrase[80];
	char *match_string;
	int multiple_records, enable_flag;
	long record_superclass, record_class, record_type;
	size_t length;

	char usage[] =
"Usage:  show autosave      -- show whether scan changes are automatically\n"
"                              saved to 'scan.dat' or not\n"
"        show bypass_limit_switch\n"
"                           -- show if the next move will bypass automatic\n"
"                              move abort on limits\n"
"        show header_prompt -- show whether scan headers are prompted for\n"
"        show overwrite     -- show whether old datafiles are automatically\n"
"                              overwritten\n"
"        show plot          -- show current plot type\n"
"        show version       -- show program version\n"
"\n"
"Commands to show record details:\n"
"\n"
"        show records       <or>  show record 'name'\n"
"        show interfaces    <or>  show interface 'name'\n"
"        show devices       <or>  show device 'name'\n"
"        show servers       <or>  show server 'name'\n"
"        show adcs          <or>  show adc 'name'\n"
"        show amplifiers    <or>  show amplifier 'name'\n"
"        show dacs          <or>  show dac 'name'\n"
"        show dinputs       <or>  show dinput 'name'\n"
"        show doutputs      <or>  show doutput 'name'\n"
"        show motors        <or>  show motor 'name'\n"
"        show mcas          <or>  show mca 'name'\n"
"        show mcses         <or>  show mcs 'name'\n"
"        show pulsers       <or>  show pulser 'name'\n"
"        show relays        <or>  show relay 'name'\n"
"        show scalers       <or>  show scaler 'name'\n"
"        show timers        <or>  show timer 'name'\n"
"        show scans         <or>  show scan 'name'\n"
"        show variables     <or>  show variable 'name'\n"
"\n"
"        show field 'name'\n";

	int status;

	multiple_records = FALSE;
	record_type = 0;
	strlcpy( record_type_phrase, "", sizeof(record_type_phrase) );
	match_string = NULL;

	switch( argc ) {
	case 3:
		multiple_records = TRUE;
		break;
	case 4:
		if ( ( strchr( argv[3], '*' ) == NULL )
		  && ( strchr( argv[3], '?' ) == NULL ) )
		{
			multiple_records = FALSE;
		} else {
			multiple_records = TRUE;

			match_string = argv[3];
		}
		break;
	default:
		fprintf(output, "Error: %s may have only 2 or 3 arguments.\n",
			cname );
		fprintf(output, "%s\n", usage);
		return FAILURE;
	}

	length = strlen(argv[2]);

	if ( length == 0 )
		length = 1;

	if ( strncmp( "version", argv[2], max(2,length) ) == 0 ) {
		fprintf( output, "MX version %s\n", mx_get_version_string() );
		return SUCCESS;

	} else if ( strncmp( "plot", argv[2], length ) == 0 ) {
		enable_flag = mx_plotting_is_enabled( motor_record_list );

		switch( enable_flag ) {
		case 0:
			fprintf( output, "  plotting is off.\n");
			break;
		case 1:
			fprintf( output, "  plotting is on.\n");
			break;
		case 2:
			fprintf( output, "  plotting is on. (nowait)\n");
			break;
		default:
			fprintf( output,
			"  plot enable flag has an unexpected value of %d\n",
				enable_flag );
			break;
		}
		return SUCCESS;

	} else if ( strncmp( "autosave", argv[2], length ) == 0 ) {
		if ( motor_autosave_on ) {
		    fprintf( output, "  'scan.dat' autosaving is on.\n");
		} else {
		    fprintf( output, "  'scan.dat' autosaving is off.\n");
		}
		return SUCCESS;

	} else if ( strncmp( "bypass_limit_switch", argv[2], length ) == 0 ) {
		if ( motor_bypass_limit_switch ) {
		    fprintf( output, "  bypass limit switches is on.\n");
		} else {
		    fprintf( output, "  bypass limit switches is off.\n");
		}
		return SUCCESS;

	} else if ( strncmp( "overwrite", argv[2], length ) == 0 ) {
		if ( motor_overwrite_on ) {
		    fprintf( output, "  data file overwriting is on.\n");
		} else {
		    fprintf( output, "  data file overwriting is off.\n");
		}
		return SUCCESS;

	} else if ( strncmp( "header_prompt", argv[2], length ) == 0 ) {
		if ( motor_header_prompt_on ) {
		    fprintf( output, "  header prompting is on.\n");
		} else {
		    fprintf( output, "  header prompting is off.\n");
		}
		return SUCCESS;

	} else if ( strncmp( "field", argv[2], length ) == 0 ) {
		if ( argc != 4 ) {
			fprintf(output, "Error: no field name specified.\n" );
			return FAILURE;
		}
		status = motor_show_field( argv[3] );
		return status;

	} else if ( strncmp( "records", argv[2], length ) == 0 ) {
		record_superclass = MXR_ANY;
		record_class = MXC_ANY;
		record_type = MXT_ANY;

		strlcpy(record_type_phrase, "!!!", sizeof(record_type_phrase));

	} else if ( strncmp( "interfaces", argv[2], length ) == 0 ) {
		record_superclass = MXR_INTERFACE;
		record_class = MXC_ANY;
		record_type = MXT_ANY;

		strlcpy( record_type_phrase, "an interface",
					sizeof(record_type_phrase) );

	} else if ( strncmp( "devices", argv[2], max(2,length) ) == 0 ) {
		record_superclass = MXR_DEVICE;
		record_class = MXC_ANY;
		record_type = MXT_ANY;

		strlcpy( record_type_phrase, "a device",
					sizeof(record_type_phrase) );

	} else if ( strncmp( "servers", argv[2], max(2,length) ) == 0 ) {
		record_superclass = MXR_SERVER;
		record_class = MXC_ANY;
		record_type = MXT_ANY;

		strlcpy( record_type_phrase, "a server",
					sizeof(record_type_phrase) );

	} else if ( strncmp( "adcs", argv[2], length ) == 0 ) {
		record_superclass = MXR_DEVICE;
		record_class = MXC_ANALOG_INPUT;
		record_type = MXT_ANY;

		strlcpy( record_type_phrase, "an ADC",
					sizeof(record_type_phrase) );

	} else if ( strncmp( "amplifiers", argv[2], length ) == 0 ) {
		record_superclass = MXR_DEVICE;
		record_class = MXC_AMPLIFIER;
		record_type = MXT_ANY;

		strlcpy( record_type_phrase, "an amplifier",
					sizeof(record_type_phrase) );

	} else if ( strncmp( "dacs", argv[2], max(2,length) ) == 0 ) {
		record_superclass = MXR_DEVICE;
		record_class = MXC_ANALOG_OUTPUT;
		record_type = MXT_ANY;

		strlcpy( record_type_phrase, "a DAC",
					sizeof(record_type_phrase) );

	} else if ( strncmp( "dinputs", argv[2], max(2,length) ) == 0 ) {
		record_superclass = MXR_DEVICE;
		record_class = MXC_DIGITAL_INPUT;
		record_type = MXT_ANY;

		strlcpy( record_type_phrase, "a digital input",
					sizeof(record_type_phrase) );

	} else if ( strncmp( "doutputs", argv[2], max(2,length) ) == 0 ) {
		record_superclass = MXR_DEVICE;
		record_class = MXC_DIGITAL_OUTPUT;
		record_type = MXT_ANY;

		strlcpy( record_type_phrase, "a digital output",
					sizeof(record_type_phrase) );

	} else if ( strncmp( "mcas", argv[2], length ) == 0 ) {
		record_superclass = MXR_DEVICE;
		record_class = MXC_MULTICHANNEL_ANALYZER;
		record_type = MXT_ANY;

		strlcpy( record_type_phrase, "an MCA",
					sizeof(record_type_phrase) );

	} else if ( strncmp( "mcses", argv[2], length ) == 0 ) {
		record_superclass = MXR_DEVICE;
		record_class = MXC_MULTICHANNEL_SCALER;
		record_type = MXT_ANY;

		strlcpy( record_type_phrase, "an MCS",
					sizeof(record_type_phrase) );

	} else if ( strncmp( "motors", argv[2], length ) == 0 ) {
		record_superclass = MXR_DEVICE;
		record_class = MXC_MOTOR;
		record_type = MXT_ANY;

		strlcpy( record_type_phrase, "a motor",
					sizeof(record_type_phrase) );

	} else if ( strncmp( "pulsers", argv[2], length ) == 0 ) {
		record_superclass = MXR_DEVICE;
		record_class = MXC_PULSE_GENERATOR;
		record_type = MXT_ANY;

		strlcpy( record_type_phrase, "a pulse generator",
					sizeof(record_type_phrase) );

	} else if ( strncmp( "relays", argv[2], length ) == 0 ) {
		record_superclass = MXR_DEVICE;
		record_class = MXC_RELAY;
		record_type = MXT_ANY;

		strlcpy( record_type_phrase, "a relay",
					sizeof(record_type_phrase) );

	} else if ( strncmp( "scalers", argv[2], max(4,length) ) == 0 ) {
		record_superclass = MXR_DEVICE;
		record_class = MXC_SCALER;
		record_type = MXT_ANY;

		strlcpy( record_type_phrase, "a scaler",
					sizeof(record_type_phrase) );

	} else if ( strncmp( "timers", argv[2], length ) == 0 ) {
		record_superclass = MXR_DEVICE;
		record_class = MXC_TIMER;
		record_type = MXT_ANY;

		strlcpy( record_type_phrase, "a timer",
					sizeof(record_type_phrase) );

	} else if ( strncmp( "scans", argv[2], max(4,length) ) == 0 ) {
		record_superclass = MXR_SCAN;
		record_class = MXC_ANY;
		record_type = MXT_ANY;

		strlcpy( record_type_phrase, "a scan",
					sizeof(record_type_phrase) );

	} else if ( strncmp( "variables", argv[2], max(2,length) ) == 0 ) {
		record_superclass = MXR_VARIABLE;
		record_class = MXC_ANY;
		record_type = MXT_ANY;

		strlcpy( record_type_phrase, "a variable",
					sizeof(record_type_phrase) );

	} else {
		fprintf(output,"show: Unrecognized option '%s'\n\n", argv[2]);
		fprintf(output,"%s\n", usage);
		return FAILURE;
	}

	if ( multiple_records ) {
		status = motor_show_records(
			record_superclass, record_class, record_type,
			match_string );
	} else {
		status = motor_show_record(
			record_superclass, record_class, record_type,
			record_type_phrase, argv[3], FALSE );
	}

	return status;
}

int
motor_show_records( long record_superclass, long record_class,
			long record_type, char *match_string )
{
	MX_RECORD *record;
	MX_RECORD_FIELD *field;
	MX_MOTOR *motor;
	int status, display_record;
	long i, field_type, num_dimensions, num_record_fields;
	void *data_ptr, *value_ptr;

	/* Loop through all the device records looking for 
	 * the requested record types.
	 */

	record = motor_record_list;

	do {
	    display_record = FALSE;

	    if ( (record_superclass == MXR_ANY
			|| record_superclass == record->mx_superclass )
	    && ( record_class == MXC_ANY || record_class == record->mx_class )
	    && ( record_type == MXT_ANY || record_type == record->mx_type ) ) {

		if ( match_string == NULL ) {
		    display_record = TRUE;
		} else {
		    if ( mx_match( match_string, record->name ) ) {

			display_record = TRUE;
		    } else {
			display_record = FALSE;
		    }
		}
	    }

	    if ( display_record == TRUE ) {

		/* Do we use the old "print_summary" driver entry point
		 * or do we use the new record field support?
		 */

		field = record->record_field_array;

		    /* Use new record field support. */

		    num_record_fields = record->num_record_fields;

		    /* Print the record name with special formatting. */

		    fprintf( output, "%-16s ", record->name );

		    /* Update the values in the record structures to
		     * be consistent with the current hardware status.
		     */

		    (void) mx_update_record_values( record );

		    /* As a special case, print out the user position
		     * and units for motor records.
		     */

		    if ( record->mx_class == MXC_MOTOR ) {
			motor = (MX_MOTOR *) record->record_class_struct;

			if ( motor == NULL ) {
			    fprintf( output, "(NULL MX_MOTOR pointer) " );
			} else {
			    if ( fabs(motor->position) < 1.0e7 ) {
				fprintf( output, "%11.3f %-5s ",
				    motor->position, motor->units );
			    } else {
				fprintf( output, "%11.3e %-5s ",
				    motor->position, motor->units );
			    }
			}
		    }

		    /* Record name is the first field so we skip over it. */

		    for ( i = 1; i < num_record_fields; i++ ) {
			/* Show only selected fields. */

			if (field[i].flags & MXFF_IN_SUMMARY) {
			    field_type = field[i].datatype;
			    num_dimensions = field[i].num_dimensions;

			    if ( (num_dimensions > 1)
				|| ((field_type != MXFT_STRING)
				&& (num_dimensions > 0)) ) {

				status = motor_print_field_array( record,
						&field[i], FALSE );
			    } else {
				data_ptr = field[i].data_pointer;

				if ( field[i].flags & MXFF_VARARGS ) {
					value_ptr =
				    mx_read_void_pointer_from_memory_location(
						data_ptr );
				} else {
					value_ptr = data_ptr;
				}

			    	status = motor_print_field_data( record,
					&field[i], value_ptr, FALSE );
			    }
			    fprintf( output, " " );
			}
		    }
		    fprintf( output, "\n" );
		    fflush( output );
	    }
	    record = record->next_record;

	    if ( record == (MX_RECORD *) NULL ) {
		fprintf( output,
	"ERROR: Next record pointer is NULL.  This should never happen!\n" );

		/* Break out of the do...while() loop, since we don't know
		 * where the next record is anyway.
		 */

		break;
	    }

	} while ( record != motor_record_list );

	return SUCCESS;
}

int
motor_show_record(
	long record_superclass,
	long record_class,
	long record_type,
	char *record_type_name,
	char *record_name,
	int show_all )
{
	MX_RECORD *record;
	MX_RECORD_FIELD *field;
	MX_RECORD_FUNCTION_LIST *flist;
	mx_status_type ( *structure_fn ) ( FILE *, MX_RECORD * );
	mx_status_type mx_status;
	int status, show_field;
	long record_superclass_match;
	long record_class_match;
	long record_type_match;
	long i, mask, field_type, num_record_fields, num_dimensions;
	void *data_ptr, *value_ptr;

	record = mx_get_record( motor_record_list, record_name );

	if ( record == (MX_RECORD *) NULL ) {
		fprintf( output,
			"Record '%s' does not exist.\n", record_name );

		return FAILURE;
	}

	if ( record_superclass == MXR_ANY ) {
		record_superclass_match = TRUE;
	} else {
		record_superclass_match
			= ( record->mx_superclass ) & record_superclass;
	}

	if ( record_class == MXC_ANY ) {
		record_class_match = TRUE;
	} else {
		record_class_match = ( record->mx_class ) & record_class;
	}

	if ( record_type == MXT_ANY ) {
		record_type_match = TRUE;
	} else {
		record_type_match  = ( record->mx_type  ) & record_type;
	}

	if ( !record_superclass_match
	  || !record_class_match
	  || !record_type_match ) {
		fprintf( output,
			"Record '%s' is not %s record.\n",
			record_name, record_type_name );

		return FAILURE;
	}

	/* Do we attempt to use the "print_structure" driver entry point? */

	flist = record->record_function_list;

	if ( flist == (MX_RECORD_FUNCTION_LIST *) NULL ) {
		fprintf( output, "(Record '%s' has a null function list)\n",
			record->name );

		return FAILURE;
	}

	structure_fn = flist->print_structure;

	if ( structure_fn != NULL && show_all == FALSE ) {
		mx_status = (*structure_fn)( output, record );

		if ( mx_status.code == MXE_SUCCESS ) {
			status = SUCCESS;
		} else {
			status = FAILURE;

			fprintf(output,
			"(Error while printing structure for record '%s')\n",
					record->name);
		}
	} else {
		/* Use new record field support. */

		field = record->record_field_array;

		if ( field == NULL ) {
			if ( show_all ) {
				fprintf( output,
"Error: Cannot invoke 'showall' using an old style record.\n" );
			} else {
				fprintf( output,
"Error: This is an old style record with a NULL 'print_structures'\n"
"       function entry point.\n" );
			}
			return FAILURE;
		}

		/* Update the values in the record structures to be
		 * consistent with the current hardware status.
		 */

		(void) mx_update_record_values( record );

		/* Now go through all the record fields looking for
		 * fields to display.
		 */

		num_record_fields = record->num_record_fields;

		fprintf( output, "Record '%s':\n\n", record->name );

		for ( i = 0; i < num_record_fields; i++ ) {

			/* Show only field listed in the description unless
			 * we were invoked by the "showall" command.
			 */

			if ( show_all ) {
				show_field = TRUE;
			} else {
				mask = MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY;

				if (field[i].flags & mask) {
					show_field = TRUE;
				} else {
					show_field = FALSE;
				}
			}

			if ( show_field ) {

				field_type = field[i].datatype;
				num_dimensions = field[i].num_dimensions;

				fprintf( output, "  %*s = ",
					-(MXU_FIELD_NAME_LENGTH + 1),
					field[i].name );

				/* Handle scalars and multidimensional
				 * arrays separately.
				 */

				if ( (num_dimensions > 1)
				  || ((field_type != MXFT_STRING)
					&& (num_dimensions > 0)) ) {

					status = motor_print_field_array(
					    record, &field[i], TRUE );
				} else {
					data_ptr = field[i].data_pointer;

					if ( field[i].flags & MXFF_VARARGS ) {
						value_ptr =
				    mx_read_void_pointer_from_memory_location(
							data_ptr );
					} else {
						value_ptr = data_ptr;
					}

					status = motor_print_field_data(
					  record, &field[i], value_ptr, TRUE);
				}

				fprintf( output, "\n" );
			}
		}
		fprintf( output, "\n" );

		status = SUCCESS;
	}

	return status;
}

int
motor_print_field_data(MX_RECORD *record, MX_RECORD_FIELD *field,
					void *data_ptr, int verbose)
{
	MX_RECORD *other_record;
	MX_INTERFACE *interface;
	MX_DRIVER *driver;
	long field_type;
	int c, i, length;
	char *typename;

	field_type = field->datatype;

	switch( field_type ) {
	case MXFT_BOOL:
		fprintf( output, "%d", *((mx_bool_type *) data_ptr) );
		break;
	case MXFT_STRING:
		fprintf( output, "\"%s\"", (char *) data_ptr );
		break;
	case MXFT_CHAR:
		fprintf( output, "'%c'", *((char *) data_ptr) );
		break;
	case MXFT_UCHAR:
		fprintf( output, "'%c'", *((unsigned char *) data_ptr) );
		break;
	case MXFT_SHORT:
		fprintf( output, "%hd", *((short *) data_ptr) );
		break;
	case MXFT_USHORT:
		fprintf( output, "%hu", *((unsigned short *) data_ptr) );
		break;
	case MXFT_LONG:
		fprintf( output, "%ld", *((long *) data_ptr) );
		break;
	case MXFT_ULONG:
		fprintf( output, "%lu", *((unsigned long *) data_ptr) );
		break;
	case MXFT_INT64:
		fprintf( output, "%" PRId64, *((int64_t *) data_ptr) );
		break;
	case MXFT_UINT64:
		fprintf( output, "%" PRIu64, *((uint64_t *) data_ptr) );
		break;
	case MXFT_FLOAT:
		fprintf( output, "%.*g", record->precision,
						*((float *) data_ptr) );
		break;
	case MXFT_DOUBLE:
		fprintf( output, "%.*g", record->precision,
						*((double *) data_ptr) );
		break;
	case MXFT_HEX:
		fprintf( output, "%#lx", *((unsigned long *) data_ptr) );
		break;
	case MXFT_RECORD:
		other_record = (MX_RECORD *)
			mx_read_void_pointer_from_memory_location( data_ptr );

		if ( other_record == (MX_RECORD *) NULL ) {
			fprintf( output, "NULL" );
		} else {
			fprintf( output, "%s", other_record->name );
		}
		break;
	case MXFT_RECORDTYPE:
		driver = (MX_DRIVER *) field->typeinfo;

		if (driver == NULL) {
			if ( verbose ) {
				fprintf(output, "NULL type list");
			} else {
				fprintf(output, "NULL");
			}
		} else {
			if ( strcmp( field->name, "type" ) == 0 ) {

				typename = driver->name;

				length = strlen( typename );

				for ( i = 0; i < length; i++ ) {
					c = (int) typename[i];

					if ( islower(c) ) {
						c = toupper(c);
					}
					fputc( c, output );
				}
			} else {
				fprintf(output, "%s", driver->name);
			}
		}
		break;
	case MXFT_INTERFACE:
		interface = (MX_INTERFACE *) data_ptr;

		fprintf( output, "%s:%s", interface->record->name,
					interface->address_name );
		break;
	default:
		if ( verbose ) {
			fprintf( output,
				"'Unknown field type %ld'", field_type );
		} else {
			fprintf( output, "??" );
		}
		break;
	}

	return SUCCESS;
}

#define MAX_ELEMENTS_SHOWN	8

int
motor_print_field_array(MX_RECORD *record, MX_RECORD_FIELD *field, int verbose)
{
	long num_dimensions, num_elements;
	long i_elements, j_elements;
	long *dimension;
	size_t *data_element_size;
	long field_type;
	char *data_ptr, *ptr, *ptr_i, *ptr_j;
	char **ptr2;
	void *array_ptr;
	int i, j, status, loop_max, i_loop_max, j_loop_max;

	num_dimensions = field->num_dimensions;
	dimension = field->dimension;
	data_element_size = field->data_element_size;

	field_type = field->datatype;
	data_ptr = field->data_pointer;

	if ( num_dimensions < 1 ) {
		fprintf( output, "_not_an_array_" );
		return SUCCESS;
	}

	array_ptr = mx_read_void_pointer_from_memory_location( data_ptr );

	if ( field->flags & MXFF_VARARGS ) {
		array_ptr = mx_read_void_pointer_from_memory_location(
				data_ptr );
	} else {
		array_ptr = data_ptr;
	}

	/* The following is not necessarily an error.  For example, an
	 * input_scan will have a NULL motor array.
	 */

	if ( array_ptr == NULL ) {
		fprintf( output, "NULL" );
		return SUCCESS;
	}

	if ( field_type == MXFT_STRING ) {
		if ( num_dimensions == 1 ) {
			status = motor_print_field_data( record, field,
					array_ptr, verbose );
		} else if ( num_dimensions == 2 ) {
			num_elements = dimension[0];

			if ( num_elements >= MAX_ELEMENTS_SHOWN ) {
				loop_max = MAX_ELEMENTS_SHOWN;
			} else {
				loop_max = num_elements;
			}
			ptr2 = (char **) array_ptr;

			for ( i = 0; i < loop_max; i++ ) {
				if ( i != 0 ) {
					fprintf(output, ",");
				}
				status = motor_print_field_data(
					record, field, ptr2[i], verbose );
			}
		} else {
			fprintf( output, "array(" );
			for ( i = 0; i < num_dimensions; i++ ) {
				if ( i != 0 ) {
					fprintf( output, "," );
				}
				fprintf( output, "%ld", dimension[i] );
			}
			fprintf( output, ")" );
		}

	} else {	/* Not a string (! MXFT_STRING) */

		if ( num_dimensions == 1 ) {
			num_elements = dimension[0];

			if ( num_elements <= 0 ) {
				if ( verbose ) {
					fprintf(output, "'num_elements = %ld'",
						num_elements);
				} else {
					fprintf(output, "?!");
				}

			} else if ( num_elements == 1 ) {
				status = motor_print_field_data(
					record, field, array_ptr, verbose );
			} else {
				if ( num_elements >= MAX_ELEMENTS_SHOWN ) {
					loop_max = MAX_ELEMENTS_SHOWN;
				} else {
					loop_max = num_elements;
				}

				fprintf(output, "(");

				ptr = array_ptr;

				for ( i = 0; i < loop_max; i++ ) {
					if ( i != 0 ) {
						fprintf(output, ",");
					}
					status = motor_print_field_data(
						record, field, ptr, verbose );

					ptr += data_element_size[0];
				}
				if ( num_elements > MAX_ELEMENTS_SHOWN ) {
					fprintf( output, ",...)" );
				} else {
					fprintf( output, ")" );
				}
			}
		} else if ( num_dimensions == 2 ) {
			i_elements = dimension[0];
			j_elements = dimension[1];

			if ( i_elements >= MAX_ELEMENTS_SHOWN ) {
				i_loop_max = MAX_ELEMENTS_SHOWN;
			} else {
				i_loop_max = i_elements;
			}
			if ( j_elements >= MAX_ELEMENTS_SHOWN ) {
				j_loop_max = MAX_ELEMENTS_SHOWN;
			} else {
				j_loop_max = j_elements;
			}

			fprintf(output, "(");

			ptr_i = array_ptr;

			for ( i = 0; i < i_loop_max; i++ ) {
				if ( i != 0 ) {
					fprintf(output, ",");
				}
				fprintf(output, "(");

				ptr_j =
			mx_read_void_pointer_from_memory_location(ptr_i);

				for ( j = 0; j < j_loop_max; j++ ) {
					if ( j != 0 ) {
						fprintf(output, ",");
					}
					status = motor_print_field_data(
						record, field, ptr_j, verbose);

					ptr_j += data_element_size[0];
				}
				if ( j_elements > MAX_ELEMENTS_SHOWN ) {
					fprintf(output, ",...)" );
				} else {
					fprintf(output, ")");
				}

				ptr_i += data_element_size[1];
			}
			if ( i_elements > MAX_ELEMENTS_SHOWN ) {
				fprintf(output, ",...)" );
			} else {
				fprintf(output, ")");
			}
		} else {
			fprintf( output, "array(" );
			for ( i = 0; i < num_dimensions; i++ ) {
				if ( i != 0 ) {
					fprintf( output, "," );
				}
				fprintf( output, "%ld", dimension[i] );
			}
			fprintf( output, ")" );
		}
	}

	return SUCCESS;
}

int
motor_show_field( char *record_field_name )
{
	static const char cname[] = "show field";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	char *ptr, *record_name, *field_name;
	void *pointer_to_value;
	mx_status_type mx_status;
	char token_buffer[80];
	mx_status_type ( *token_constructor )
		(void *, char *, size_t, MX_RECORD *, MX_RECORD_FIELD *);

#if 0
	fprintf( output, "'%s' invoked with argument '%s'\n",
		cname, record_field_name );
#endif

	record_name = record_field_name;

	ptr = strchr( record_field_name, '.' );

	if ( ptr == NULL ) {
		fprintf(output,
	"%s: No period found in '%s' to separate the record name part\n",
			cname, record_field_name );
		fprintf(output, "from the field name part.\n" );

		return FAILURE;
	}

	*ptr = '\0';

	field_name = ++ptr;

#if 0
	fprintf(output,"%s: record_name = '%s'\n", cname, record_name);
	fprintf(output,"%s: field_name = '%s'\n", cname, field_name);
#endif

	record = mx_get_record( motor_record_list, record_name );

	if ( record == (MX_RECORD *) NULL ) {
		fprintf(output, "%s: Unknown record '%s'\n",cname,record_name);
		return FAILURE;
	} 

	mx_status = mx_find_record_field(
				record, field_name, &record_field );

	if ( mx_status.code != MXE_SUCCESS )
		return FAILURE;

	if ( record_field->flags & MXFF_VARARGS ) {
		pointer_to_value
			= mx_read_void_pointer_from_memory_location(
				record_field->data_pointer );
	} else {
		pointer_to_value = record_field->data_pointer;
	}

	mx_status = mx_get_token_constructor(
			record_field->datatype, &token_constructor );

	if ( mx_status.code != MXE_SUCCESS )
		return FAILURE;

	/* Construct the string description of this field. */

	strlcpy( token_buffer, "", sizeof(token_buffer) );

	if ( (record_field->num_dimensions == 0)
	  || ((record_field->datatype == MXFT_STRING)
	     && (record_field->num_dimensions == 1)) ) {

		mx_status = (*token_constructor)( pointer_to_value,
				token_buffer, sizeof( token_buffer ) - 1,
				record, record_field );

	} else {
		mx_status = mx_create_array_description( pointer_to_value, 0,
				token_buffer, sizeof( token_buffer ) - 1,
				record, record_field, token_constructor );
	}

	if ( mx_status.code != MXE_SUCCESS )
		return FAILURE;

	/* There may be an extra space in front if we invoked
	 * mx_create_array_description().
	 */

	ptr = &token_buffer[0];

	if ( *ptr == ' ' ) {
		ptr++;
	}

#if 0
	fprintf(output, "%s: Constructed token buffer = '%s'\n",
						cname, ptr);
#endif

	fprintf( output, "%s\n", ptr );

	return SUCCESS;
}

