/*
 * Name:    mshow.c
 *
 * Purpose: Motor show parameters functions.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999-2004, 2006-2007, 2009-2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <time.h>

#include "mx_inttypes.h"
#include "mx_amplifier.h"
#include "mx_analog_input.h"
#include "mx_analog_output.h"
#include "mx_digital_input.h"
#include "mx_digital_output.h"
#include "mx_scaler.h"
#include "mx_timer.h"
#include "mx_plot.h"
#include "mx_array.h"
#include "mx_memory.h"
#include "mx_socket.h"
#include "mx_process.h"
#include "mx_time.h"
#include "mx_version.h"
#include "motor.h"

#ifndef max
#define max(a,b) ((a) >= (b) ? (a) : (b))
#endif

void motor_show_version( void );

/*---*/

int
motor_showall_fn( int argc, char *argv[] )
{
	MX_RECORD *record;
	int status;
	size_t length;
	mx_status_type mx_status;

	char usage[] = 
		"Usage:  showall record 'record_name'\n"
		"        showall update 'record_name'\n"
		"        showall fielddef 'record_name'\n";

	if ( argc != 4 ) {
		fprintf(output, "%s", usage);
		return FAILURE;
	}

	length = strlen(argv[2]);

	if ( length == 0 )
		length = 1;

	if ( strncmp( "record", argv[2], length ) == 0 ) {
		status = motor_show_record(
		    MXR_ANY, MXC_ANY, MXT_ANY, argv[2], argv[3], TRUE, FALSE );
	} else
	if ( strncmp( "update", argv[2], length ) == 0 ) {
		status = motor_show_record(
		    MXR_ANY, MXC_ANY, MXT_ANY, argv[2], argv[3], TRUE, TRUE );
	} else
	if ( strncmp( "fielddef", argv[2], 6 ) == 0 ) {
		record = mx_get_record( motor_record_list, argv[3] );

		if ( record == (MX_RECORD *) NULL ) {
			fprintf( output, "showall: record '%s' not found.\n",
					argv[3] );
			return FAILURE;
		}

		mx_status = mx_print_all_field_definitions( output, record );

		if ( mx_status.code == MXE_SUCCESS ) {
			return SUCCESS;
		} else {
			return FAILURE;
		}
	} else {
		fprintf(output,
			"showall: Unrecognized option '%s'\n\n", argv[2]);

		fprintf(output, "%s", usage);
		return FAILURE;
	}

	return status;
}

int
motor_show_fn( int argc, char *argv[] )
{
	static const char cname[] = "show";

	MX_PROCESS_MEMINFO process_meminfo;
	MX_SYSTEM_MEMINFO system_meminfo;
	MX_LIST_HEAD *list_head;
	char record_type_phrase[80];
	char *match_string;
	int multiple_records, enable_flag;
	long record_superclass, record_class, record_type;
	size_t length;
	mx_status_type mx_status;

	char usage[] =
"Usage:  show autosave      -- show whether scan changes are automatically\n"
"                              saved to 'mxscan.dat' or not\n"
"        show bypass_limit_switch\n"
"                           -- show if the next move will bypass automatic\n"
"                              move abort on limits\n"
"        show debug         -- show MX debugging level\n"
"        show header_prompt -- show whether scan headers are prompted for\n"
"        show netdebug      -- show MX network debugging flags\n"
"        show overwrite     -- show whether old datafiles are automatically\n"
"                              overwritten\n"
"        show plot          -- show current plot type\n"
"        show scanlog       -- show whether scan progress messages are\n"
"                              displayed\n"
"        show version       -- show program version\n"
"        show memory        -- show process memory usage\n"
"        show system        -- show system memory usage\n"
"\n"
"Commands to show record details:\n"
"\n"
"        show records        <or>  show record 'name'\n"
"        show interfaces     <or>  show interface 'name'\n"
"        show devices        <or>  show device 'name'\n"
"        show servers        <or>  show server 'name'\n"
"\n"
"        show adcs           <or>  show adc 'name'\n"
"        show amplifiers     <or>  show amplifier 'name'\n"
"        show area_detectors <or>  show area_detector 'name'\n"
"        show dacs           <or>  show dac 'name'\n"
"        show dinputs        <or>  show dinput 'name'\n"
"        show doutputs       <or>  show doutput 'name'\n"
"        show mcas           <or>  show mca 'name'\n"
"        show mces           <or>  show mce 'name'\n"
"        show mcses          <or>  show mcs 'name'\n"
"        show motors         <or>  show motor 'name'\n"
"        show pulsers        <or>  show pulser 'name'\n"
"        show relays         <or>  show relay 'name'\n"
"        show scas           <or>  show sca 'name'\n"
"        show scalers        <or>  show scaler 'name'\n"
"        show timers         <or>  show timer 'name'\n"
"        show scans          <or>  show scan 'name'\n"
"        show variables      <or>  show variable 'name'\n"
"\n"
"        show field 'recordname.fieldname'\n";

	int status;

	multiple_records = FALSE;
	record_superclass = 0;
	record_class = 0;
	record_type = 0;
	strlcpy( record_type_phrase, "", sizeof(record_type_phrase) );
	match_string = NULL;

	list_head = mx_get_record_list_head_struct( motor_record_list );

	if ( list_head == (MX_LIST_HEAD *) NULL ) {
		fprintf( output,
	  "The MX_LIST_HEAD pointer is NULL!!!  This should be impossible!\n" );
		return FAILURE;
	}

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
		motor_show_version();
		return SUCCESS;

	} else if ( strncmp( "help", argv[2], length ) == 0 ) {
		fprintf( output, "%s\n", usage );
		return FAILURE;

	} else if ( strncmp( "date", argv[2], max(2,length) ) == 0 ) {
		time_t time_struct;
		struct tm current_time;
		char time_buffer[80];

		time( &time_struct );

		(void) localtime_r( &time_struct, &current_time );

		strftime( time_buffer, sizeof(time_buffer),
			"%G %b %d %H:%M:%S\n", &current_time );

		fputs( time_buffer, output );
		return SUCCESS;

	} else if ( strncmp( "memory", argv[2], length ) == 0 ) {

		mx_status = mx_get_process_meminfo( MXF_PROCESS_ID_SELF,
							&process_meminfo );
		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		mx_display_process_meminfo( &process_meminfo );

	} else if ( strncmp( "system", argv[2], length ) == 0 ) {

		mx_status = mx_get_system_meminfo( &system_meminfo );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		mx_display_system_meminfo( &system_meminfo );

	} else if ( strncmp( "plot", argv[2], length ) == 0 ) {
		enable_flag = mx_plotting_is_enabled( motor_record_list );

		switch( enable_flag ) {
		case MXPF_PLOT_OFF:
			fprintf( output, "  plotting is off.\n");
			break;
		case MXPF_PLOT_ON:
			fprintf( output, "  plotting is on.\n");
			break;
		case MXPF_PLOT_NOWAIT:
			fprintf( output, "  plotting is on. (nowait)\n");
			break;
		case MXPF_PLOT_END:
			fprintf( output, "  plotting is on. (at end)\n");
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
		    fprintf( output, "  'mxscan.dat' autosaving is on.\n");
		} else {
		    fprintf( output, "  'mxscan.dat' autosaving is off.\n");
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

	} else if ( strncmp( "debug", argv[2], length ) == 0 ) {
		fprintf( output, "  debug level = %d\n",
			mx_get_debug_level() );

		return SUCCESS;

	} else if ( strncmp( "netdebug", argv[2], length ) == 0 ) {
		if ( list_head->network_debug_flags
			& MXF_NETWORK_SERVER_DEBUG_ANY )
		{
			fprintf( output, "  network debug = on\n" );
		} else {
			fprintf( output, "  network debug = off\n" );
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

	} else if ( strncmp( "area_detectors", argv[2], length ) == 0 ) {
		record_superclass = MXR_DEVICE;
		record_class = MXC_AREA_DETECTOR;
		record_type = MXT_ANY;

		strlcpy( record_type_phrase, "an area detector",
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

	} else if ( strncmp( "mces", argv[2], length ) == 0 ) {
		record_superclass = MXR_DEVICE;
		record_class = MXC_MULTICHANNEL_ENCODER;
		record_type = MXT_ANY;

		strlcpy( record_type_phrase, "an MCE",
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

	} else if ( strncmp( "scas", argv[2], max(4,length) ) == 0 ) {
		record_superclass = MXR_DEVICE;
		record_class = MXC_SINGLE_CHANNEL_ANALYZER;
		record_type = MXT_ANY;

		strlcpy( record_type_phrase, "an SCA",
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

	} else if ( strncmp( "scans", argv[2], max(3,length) ) == 0 ) {
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

	} else if ( strncmp( "scanlog", argv[2], length ) == 0 ) {
		if ( mx_get_scanlog_enable() ) {
		    fprintf( output, "  scan log messages are on.\n");
		} else {
		    fprintf( output, "  scan log messages are off.\n");
		}
		return SUCCESS;

	} else {
		fprintf(output,"show: Unrecognized option '%s'\n\n", argv[2]);
		return FAILURE;
	}

	if ( multiple_records ) {
		status = motor_show_records(
			record_superclass, record_class, record_type,
			match_string );
	} else {
		status = motor_show_record(
			record_superclass, record_class, record_type,
			record_type_phrase, argv[3], FALSE, FALSE );
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
	int display_record;
	long i, field_type, num_dimensions, num_record_fields;
	void *value_ptr;

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

				(void) mx_print_field_array( output,
						record, &field[i], FALSE );
			    } else {
			    	value_ptr =
					mx_get_field_value_pointer( &field[i] );

				if ( (record->mx_class == MXC_AMPLIFIER)
				  && (field[i].label_value == MXLV_AMP_GAIN) )
				{
					/* Amplifier gains are treated 
					 * specially.
					 */

					double gain, diff;
					long rounded_gain;

					gain = *((double *) value_ptr);

					if ( gain > 9999.99999999 ) {
						fprintf( output, "%.3e", gain );
					} else {
						rounded_gain = mx_round(gain);

						diff = mx_difference( gain,
						  (double) rounded_gain );

						if ( diff < 1.0e-12 ) {
							fprintf( output,
							"%ld", rounded_gain );
						} else {
							fprintf( output,
							"%.3f", gain );
						}
					}
				} else {
				    	(void) mx_print_field_value( output,
					record, &field[i], value_ptr, FALSE );
				}
			    }
			    fprintf( output, " " );
			}
		    }
		    fprintf( output, "\n" );
		    fflush( output );
	    }
	    if ( record->next_record == (MX_RECORD *) NULL ) {
		fprintf( output,
		"ERROR: Next record pointer for record '%s' is NULL.  "
		"This should never happen!\n", record->name );

		/* Break out of the do...while() loop, since we don't know
		 * where the next record is anyway.
		 */

		break;
	    }

	    record = record->next_record;

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
	int show_all,
	int update_all )
{
	MX_RECORD *record;
	mx_status_type mx_status;
	long record_superclass_match;
	long record_class_match;
	long record_type_match;
	unsigned long mask;

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

	if ( show_all ) {
		mask = MXFF_SHOW_ALL;
	} else {
		mask = MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY;
	}

	if ( update_all ) {
		mask |= MXFF_UPDATE_ALL;

		mx_status = mx_initialize_record_processing( record );

		if ( mx_status.code != MXE_SUCCESS ) {
			return FAILURE;
		}
	}

	mx_status = mx_print_structure( output, record, mask );

	if ( mx_status.code != MXE_SUCCESS ) {
		return FAILURE;
	} else {
		return SUCCESS;
	}
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

	if ( record_field->flags & MXFF_NO_ACCESS ) {
		fprintf( output,
	"You are not permitted to access the record field '%s.%s'.\n",
			record_name, field_name );
		return FAILURE;
	}

	if ( record_field->flags & MXFF_VARARGS ) {
		pointer_to_value
			= mx_read_void_pointer_from_memory_location(
				record_field->data_pointer );
	} else {
		pointer_to_value = record_field->data_pointer;
	}

#if MOTOR_PROCESS_FIELDS
	mx_status = mx_initialize_record_processing( record );

	if ( mx_status.code != MXE_SUCCESS )
		return FAILURE;

	mx_status = mx_process_record_field( record, record_field,
						MX_PROCESS_GET, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return FAILURE;

#endif /* MOTOR_PROCESS_FIELDS */

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

/*=========================================================================*/

void
motor_show_version( void )
{
	MX_LIST_HEAD *list_head;
	char os_version_string[80];
	char architecture_type[80];
	char architecture_subtype[80];
	const char *revision_string;
	unsigned long num_cores;
	mx_status_type mx_status;

	list_head = mx_get_record_list_head_struct( motor_record_list );

	revision_string = mx_get_revision();

	if ( strlen( revision_string ) > 0 ) {
		fprintf( output, "MX version: %s   [ Revision: %s ]\n",
			mx_get_version_full_string(), revision_string );
	} else {
		fprintf( output, "MX version: %s\n",
			mx_get_version_full_string() );
	}

	mx_status = mx_get_os_version_string( os_version_string,
						sizeof( os_version_string ) );

	if ( mx_status.code != MXE_SUCCESS ) {
		fprintf( output, "Error: Could not get OS version string.\n" );
	} else {
		fprintf( output, "OS version: %s\n", os_version_string );
	}

/*-------------------------------------------------------------------------*/

#if defined(OS_MACOSX)
	fprintf( output, "MacOS X version: " );
	fflush( output );

	{
		unsigned long process_id;

		(void) mx_spawn( "sw_vers -productVersion", 0, &process_id );

		(void) mx_wait_for_process_id( process_id, NULL );
	}
#endif

/*-------------------------------------------------------------------------*/

#if ( defined(__GNUC__) && ( ! defined(__clang__) ) )

#  if !defined(__GNUC_PATCHLEVEL__)
#    define __GNUC_PATCHLEVEL__ 0
#  endif
	fprintf( output, "GCC version: %d.%d.%d\n",
		__GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__ );

#endif /* __GNUC__ */

/*-------------------------------------------------------------------------*/

#if defined(__clang__)

	fprintf( output, "Clang version: %d.%d.%d\n",
		__clang_major__, __clang_minor__, __clang_patchlevel__ );

#endif /* __clang__ */

/*-------------------------------------------------------------------------*/

#if defined(__GLIBC__)

#  if (__GLIBC__ < 2)
#     define MXP_USE_GNU_GET_LIBC_VERSION   0
#  elif (__GLIBC__ == 2)
#     if (__GLIBC_MINOR__ < 3)
#        define MXP_USE_GNU_GET_LIBC_VERSION   0
#     else
#        define MXP_USE_GNU_GET_LIBC_VERSION   1
#     endif
#  else
#     define MXP_USE_GNU_GET_LIBC_VERSION   1
#  endif

#  if MXP_USE_GNU_GET_LIBC_VERSION
#	include <gnu/libc-version.h>

	fprintf( output, "GLIBC version: %s\n", gnu_get_libc_version() );
#  else
	fprintf( output, "GLIBC version: %d.%d.0\n",
				__GLIBC__, __GLIBC_MINOR__ );
#  endif
#endif /* __GLIBC__ */

/*-------------------------------------------------------------------------*/

#if defined(_MSC_VER)

	fprintf( output, "Microsoft Visual C/C++ version: %d.%d\n",
		(_MSC_VER / 100), (_MSC_VER % 100) );

#endif /* _MSC_VER */

/*-------------------------------------------------------------------------*/

	list_head = mx_get_record_list_head_struct( motor_record_list );

	if ( list_head->cflags != NULL ) {
		fprintf( output, "\nCFLAGS: '%s'\n",
			list_head->cflags );
	}

/*-------------------------------------------------------------------------*/

	mx_status = mx_get_cpu_architecture( architecture_type,
					sizeof(architecture_type),
					architecture_subtype,
					sizeof(architecture_subtype) );

	fprintf( output, "\nCPU type: '%s', subtype: '%s'\n",
		architecture_type, architecture_subtype );

/*-------------------------------------------------------------------------*/

	mx_status = mx_get_number_of_cpu_cores( &num_cores );

	if ( mx_status.code != MXE_SUCCESS ) {
		fprintf( output,
			"Error: Could not get number of CPU cores.\n" );
	} else {
		fprintf( output, "CPU cores: %lu\n", num_cores );
	}

	fprintf( output, "Current CPU number: %lu\n",
				mx_get_current_cpu_number() );

	fprintf( output, "\nRevision timestamp: %" PRIu64 " seconds \n",
		list_head->mx_version_time );


	fprintf( output, "\nCurrent Posix time: %lu seconds\n",
					(unsigned long) time( NULL ) );

	fprintf( output, "\n" );

	return;
}

