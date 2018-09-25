/*
 * Name:    mset.c
 *
 * Purpose: Motor set functions.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999-2003, 2006-2007, 2009-2010, 2013, 2015, 2017-2018
 *    Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "mx_analog_input.h"
#include "mx_analog_output.h"
#include "mx_digital_input.h"
#include "mx_digital_output.h"
#include "mx_motor.h"
#include "mx_encoder.h"
#include "mx_scaler.h"
#include "mx_timer.h"
#include "mx_relay.h"
#include "mx_amplifier.h"
#include "mx_autoscale.h"
#include "mx_pulse_generator.h"
#include "mx_variable.h"
#include "mx_plot.h"
#include "d_auto_scaler.h"

#include "mx_array.h"
#include "mx_socket.h"
#include "mx_process.h"
#include "mx_multi.h"
#include "motor.h"
#include "mdialog.h"

int
motor_set_fn( int argc, char *argv[] )
{
	static const char cname[] = "set";

	MX_RECORD *record;

	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD_PARSE_STATUS parse_status;

	MX_AUTOSCALE_SCALER *autoscale_scaler;
	MX_RECORD *autoscale_record;

	char *record_name, *field_name, *ptr, *separator_found, *endptr;
	void *pointer_to_value;
	long i;
	static char buffer[ MXU_RECORD_DESCRIPTION_LENGTH + 1 ];
	static char token_buffer[ MXU_RECORD_DESCRIPTION_LENGTH + 1 ];
	mx_status_type ( *token_parser )
		( void *, char *, MX_RECORD *, MX_RECORD_FIELD *,
		MX_RECORD_FIELD_PARSE_STATUS * );

	size_t length2, length3, string_length;
	int status;
	int int_value, debug_level, num_items;
	unsigned long do_netdebug;
	long long_value;
	double double_value;
	mx_status_type mx_status;

	char separators[] = MX_RECORD_FIELD_SEPARATORS;

	static char usage[] =
"Usage:  set motor 'motorname' position 'position_value'\n"
"        set motor 'motorname' position steps 'step_value'\n"
"        set motor 'motorname' zero\n"
"        set amplifier 'amplifiername' gain 'gain_value'\n"
"        set amplifier 'amplifiername' offset 'offset_value'\n"
"        set amplifier 'amplifiername' time_constant 'time_constant_value'\n"
"        set amplifier 'amplifiername' sensitivity 'sensitivity_value'\n"
"        set autoscale 'scalername' low_limit high_limit low_dead high_dead\n"
"        set pulser 'pulsername' period width number delay function trigger\n"
"        set device 'devicename' 'value'\n"
"        set variable 'variablename' 'value'\n"
"        set field 'recordname.fieldname' 'value'\n"
"        set autosave { on | off }\n"
"        set bypass_limit_switch { on | off }\n"
"        set header_prompt { on | off }\n"
"        set overwrite { on | off }\n"
"        set plot { on | off | nowait | end }\n"
"        set scanlog { on | off }\n"
"        set debug 'debug-value'"
"        set netdebug 'network-debug-value'\n";

	if ( argc == 2 ) {
		fprintf(output,"%s\n",usage);
		return FAILURE;
	}

	length2 = strlen( argv[2] );

	if ( length2 == 0 )
		length2 = 1;

	/* SET MOTOR function. */

	if ( strncmp( argv[2], "motor", length2 ) == 0 ) {
		if ( argc <= 4 ) {
			fprintf(output,"%s\n", usage);
			return FAILURE;
		} else {
			/* Get the record corresponding to this name. */

			record = mx_get_record( motor_record_list, argv[3] );

			if ( record == (MX_RECORD *) NULL ) {
				fprintf(output,
					"Unknown motor '%s'\n", argv[3]);
				return FAILURE;
			} 

			/* Is this really a motor record? */

			if ( record->mx_class != MXC_MOTOR ) {
				fprintf(output,
				"Record '%s' is not a motor.\n", argv[3]);

				return FAILURE;
			}

			/* Strip 'motor set motor motorname'
			 * off the front of argc & argv[] before
			 * passing them to motor_set_motor().
			 */

			argc -= 4;
			argv += 4;

			status = motor_set_motor( record, argc, argv );

			if ( status != SUCCESS ) {
				fprintf( output,
					"'set motor' command failed.\n\n");

				return status;
			}
		}

	/* SET AMPLIFIER function. */

	} else if ( strncmp( argv[2], "amplifier", length2 ) == 0 ) {
		if ( argc <= 4 ) {
			fprintf(output,"%s\n", usage);
			return FAILURE;
		} else {
			/* Get the record corresponding to this name. */

			record = mx_get_record( motor_record_list, argv[3] );

			if ( record == (MX_RECORD *) NULL ) {
				fprintf(output,
					"Unknown amplifier '%s'\n", argv[3]);
				return FAILURE;
			} 

			/* Is this really an amplifier record? */

			if ( record->mx_class != MXC_AMPLIFIER ) {
				fprintf(output,
				"Record '%s' is not a amplifier.\n", argv[3]);

				return FAILURE;
			}

			/* Strip 'motor set amplifier amplifiername'
			 * off the front of argc & argv[] before
			 * passing them to motor_set_amplifier().
			 */

			argc -= 4;
			argv += 4;

			status = motor_set_amplifier( record, argc, argv );

			if ( status != SUCCESS ) {
				fprintf( output,
				"'set amplifier' command failed.\n\n");

				return status;
			}
		}

	/* SET AUTOSCALE function. */

	} else if ( strncmp( argv[2], "autoscale", length2 ) == 0 ) {
		if ( argc != 8 ) {
			fprintf(output,"%s\n", usage);
			return FAILURE;
		} else {
			/* Get the record corresponding to this name. */

			record = mx_get_record( motor_record_list, argv[3] );

			if ( record == (MX_RECORD *) NULL ) {
				fprintf(output,
					"Unknown autoscale '%s'\n", argv[3]);
				return FAILURE;
			} 

			/* You are allowed to either specify the autoscale
			 * record itself or the name of the autoscaled scaler
			 * that it depends on.
			 */

			if ( record->mx_superclass != MXR_DEVICE ) {
				fprintf(output,
				"Record '%s' is not an autoscaling record.\n",
					record->name);

				return FAILURE;
			}

			switch( record->mx_class ) {
			case MXC_AUTOSCALE:
				autoscale_record = record;
				break;

			case MXC_SCALER:
				switch( record->mx_type ) {
				case MXT_SCL_AUTOSCALE:
					autoscale_scaler =
						(MX_AUTOSCALE_SCALER *)
						record->record_type_struct;

					autoscale_record =
					    autoscale_scaler->autoscale_record;

					break;
				default:
					fprintf(output,
				"Record '%s' is not an autoscaling record.\n",
						record->name);

					return FAILURE;
				}
				break;
			default:
				fprintf(output,
				"Record '%s' is not an autoscaling record.\n",
					record->name);

				return FAILURE;
			}

			mx_status = mx_autoscale_set_limits( autoscale_record,
				atof( argv[4] ), atof( argv[5] ),
				atof( argv[6] ), atof( argv[7] ) );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;
		}

	/* SET PULSER function. */

	} else if ( strncmp( argv[2], "pulser", length2 ) == 0 ) {
		if ( argc != 10 ) {
			fprintf(output,"%s\n", usage);
			return FAILURE;
		} else {
			/* Get the record corresponding to this name. */

			record = mx_get_record( motor_record_list, argv[3] );

			if ( record == (MX_RECORD *) NULL ) {
				fprintf(output,
				"Unknown pulse generator '%s'\n", argv[3]);
				return FAILURE;
			} 

			mx_status = mx_pulse_generator_set_pulse_period(
						record, atof( argv[4] ) );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			mx_status = mx_pulse_generator_set_pulse_width(
						record, atof( argv[5] ) );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			mx_status = mx_pulse_generator_set_num_pulses(
						record, atoi( argv[6] ) );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			mx_status = mx_pulse_generator_set_pulse_delay(
						record, atof( argv[7] ) );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			mx_status = mx_pulse_generator_set_function_mode(
						record, atoi( argv[8] ) );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			mx_status = mx_pulse_generator_set_trigger_mode(
						record, atoi( argv[9] ) );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;
		}

	/* SET DEVICE function. */

	} else if ( strncmp( argv[2], "device", length2 ) == 0 ) {
		if ( argc <= 4 ) {
			fprintf(output,"%s\n", usage);
			return FAILURE;
		} else {
			/* Get the record corresponding to this name. */

			record = mx_get_record( motor_record_list, argv[3] );

			if ( record == (MX_RECORD *) NULL ) {
				fprintf(output,
					"Unknown device '%s'\n", argv[3]);
				return FAILURE;
			} 

			switch ( record->mx_class ) {
			case MXC_ANALOG_INPUT:
				fprintf(output,
		"'%s' is an input device and cannot be written to.\n",
					record->name);
				break;

			case MXC_ANALOG_OUTPUT:
				double_value = atof( argv[4] );

				mx_status = mx_analog_output_write(
						record, double_value );
				break;

			case MXC_DIGITAL_INPUT:
				mx_status = mx_digital_input_clear( record );
				break;

			case MXC_DIGITAL_OUTPUT:
				long_value = (long)
					strtoul( argv[4], &endptr, 0 );

				if ( *endptr != '\0' ) {
					fprintf(output,
		"Non-numeric characters found in digital output value '%s'\n",
						argv[4] );

					return FAILURE;
				}

				mx_status = mx_digital_output_write(
						record, long_value );
				break;

			case MXC_MOTOR:
				double_value = atof( argv[4] );

				mx_status = mx_motor_set_position(
						record, double_value );

				if ( mx_status.code != MXE_SUCCESS ) {
					return FAILURE;
				}
				break;

			case MXC_ENCODER:
				long_value = atol( argv[4] );

				mx_status = mx_encoder_write( record,
								long_value );

				if ( mx_status.code != MXE_SUCCESS ) {
					return FAILURE;
				}
				break;

			case MXC_SCALER:
				long_value = atol( argv[4] );

				mx_status = mx_scaler_start(
							record, long_value );
				break;
 
			case MXC_TIMER:
				double_value = atof( argv[4] );

				mx_status = mx_timer_start(
							record, double_value );

				break;

			case MXC_RELAY:
				int_value = atoi( argv[4] );

				if ( int_value != 0 )
					int_value = 1;

				mx_status = mx_relay_command(
							record, int_value );

				if ( mx_status.code != MXE_SUCCESS ) {
					return FAILURE;
				}
				break;

			case MXC_AMPLIFIER:
				double_value = atof( argv[4] );

				mx_status = mx_amplifier_set_gain(
							record, double_value );

				if ( mx_status.code != MXE_SUCCESS ) {
					return FAILURE;
				}
				break;

			default:
				fprintf( stderr,
					"Unsupported device class %ld.\n",
						record->mx_class );	
			}
		}

	/* SET FIELD function. */

	} else if ( strncmp( argv[2], "field", length2 ) == 0 ) {

		if ( argc <= 4 ) {
			fprintf(output,
			"Usage: set field 'recordname.fieldname' 'value'\n");

			return FAILURE;
		}

		record_name = argv[3];

		ptr = strchr( argv[3], '.' );

		if ( ptr == NULL ) {
			fprintf(output,
		"No period found in '%s' to separate the record name part\n",
				argv[3] );
			fprintf(output, "from the field name part.\n" );

			return FAILURE;
		}

		*ptr = '\0';

		field_name = ++ptr;

#if 0
		fprintf(output,"set field: record_name = '%s'\n", record_name);
		fprintf(output,"set field: field_name = '%s'\n", field_name);
#endif

		record = mx_get_record( motor_record_list, record_name );

		if ( record == (MX_RECORD *) NULL ) {
			fprintf(output, "Unknown record '%s'\n", record_name);
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

		if ( record_field->flags & MXFF_READ_ONLY ) {
			fprintf( output,
		"You are not permitted to change the record field '%s.%s'.\n",
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

		mx_status = mx_get_token_parser(
				record_field->datatype, &token_parser );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		/* Concatenate all the rest of the arguments together. */

		strlcpy( buffer, "", sizeof(buffer) );

		for ( i = 4; i < argc; i++ ) {
			string_length = strlen( buffer );

			ptr = buffer + string_length;

			separator_found = strpbrk( argv[i],
						MX_RECORD_FIELD_SEPARATORS );

			if ( separator_found ) {
				snprintf( ptr, sizeof(buffer) - string_length,
					"\"%s\" ", argv[i] );
			} else {
				snprintf( ptr, sizeof(buffer) - string_length,
					"%s ", argv[i] );
			}
#if 0
			fprintf( output, "set field: ptr = '%s'\n", ptr );
#endif
		}
#if 0
		fprintf( output, "set field: buffer = '%s'\n", buffer );
#endif

		mx_initialize_parse_status( &parse_status, buffer, separators );

		/* If this is a string field, figure out the maximum length
		 * of a string token for the token parser.
		 */

		if ( record_field->datatype == MXFT_STRING ) {
			parse_status.max_string_token_length = 
				mx_get_max_string_token_length( record_field );
		} else {
			parse_status.max_string_token_length = 0L;
		}

		/* Parse the tokens. */

		if ( (record_field->num_dimensions == 0)
		  || ((record_field->datatype == MXFT_STRING)
		     && (record_field->num_dimensions == 1)) ) {

#if 0
			fprintf( output, "set field: saw single token\n");
#endif

			mx_status = mx_get_next_record_token( &parse_status,
					token_buffer, sizeof(token_buffer) );

#if 0
			fprintf( output, "set field: token_buffer = '%s'\n",
					token_buffer);
#endif

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			mx_status = ( *token_parser ) ( pointer_to_value,
					token_buffer, record, record_field,
					&parse_status );
		} else {
#if 0
			fprintf( output, "set field: saw array description\n");
#endif
			mx_status = mx_parse_array_description(
					pointer_to_value,
					(record_field->num_dimensions - 1),
					record, record_field,
					&parse_status, token_parser );
		}

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

#if MOTOR_PROCESS_FIELDS
		mx_status = mx_initialize_record_processing( record );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		mx_status = mx_process_record_field( record, record_field,
						MX_PROCESS_PUT, NULL );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

#endif /* MOTOR_PROCESS_FIELDS */

	/* SET VARIABLE function. */

	} else if ( strncmp( argv[2], "variable", length2 ) == 0 ) {

		if ( argc <= 4 ) {
			fprintf(output,
			"Usage: set variable 'variable_name' 'value'\n");

			return FAILURE;
		}

		/* Get the record corresponding to this name. */

		record = mx_get_record( motor_record_list, argv[3] );

		if ( record == (MX_RECORD *) NULL ) {
			fprintf(output, "Unknown variable '%s'\n", argv[3]);
			return FAILURE;
		} 

		/* Is this really a variable? */

		if ( record->mx_superclass != MXR_VARIABLE ) {
			fprintf(output,
				"Record '%s' is not a variable.\n", argv[3]);

			return FAILURE;
		}

		snprintf( buffer, sizeof(buffer),
			"set field %s.value ", argv[3] );

		MX_DEBUG( 2,("set variable: argc = %d", argc));

		for ( i = 4; i < argc; i++ ) {
			MX_DEBUG( 2,("set variable: argv[%ld] = '%s'",
							i, argv[i]));

			string_length = strlen( buffer );

			ptr = buffer + string_length;

			separator_found = strpbrk( argv[i],
						MX_RECORD_FIELD_SEPARATORS );

			if ( separator_found ) {
				snprintf( ptr, sizeof(buffer) - string_length,
					"\"%s\" ", argv[i] );
			} else {
				snprintf( ptr, sizeof(buffer) - string_length,
					"%s ", argv[i] );
			}
#if 0
			fprintf( output, "set variable: ptr = '%s'\n", ptr );
#endif
		}

		MX_DEBUG( 2,("set variable: Translated command line = '%s'",
			buffer ));

		status = cmd_execute_command_line( command_list_length,
				command_list, buffer );

		if ( status != SUCCESS )
			return status;

#if ( MOTOR_PROCESS_FIELDS == FALSE )

		/* Send the value of the variable to the remote server,
		 * if any.
		 */

		mx_status = mx_send_variable( record );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

#endif /* MOTOR_PROCESS_FIELDS */

	/* SET LABEL function. */

	} else if ( strncmp( argv[2], "label", length2 ) == 0 ) {

		snprintf( buffer, sizeof(buffer),
			"set field %s.label ", argv[3] );

		MX_DEBUG( 2,("set label: argc = %d", argc));

		for ( i = 4; i < argc; i++ ) {
			MX_DEBUG( 2,("set label: argv[%ld] = '%s'",
							i, argv[i]));

			string_length = strlen( buffer );

			ptr = buffer + string_length;

			separator_found = strpbrk( argv[i],
						MX_RECORD_FIELD_SEPARATORS );

			if ( separator_found ) {
				snprintf( ptr, sizeof(buffer) - string_length,
					"\"%s\" ", argv[i] );
			} else {
				snprintf( ptr, sizeof(buffer) - string_length,
					"%s ", argv[i] );
			}
#if 0
			fprintf( output, "set label: ptr = '%s'\n", ptr );
#endif
		}

		MX_DEBUG( 2,("set label: Translated command line = '%s'",
			buffer ));

		status = cmd_execute_command_line( command_list_length,
				command_list, buffer );

		if ( status != SUCCESS )
			return status;

	/* SET PLOT function. */

	} else if ( strncmp( argv[2], "plot", length2 ) == 0 ) {

		if ( argc <= 3 ) {
			fprintf(output,
"Usage: 'set plot on', 'set plot off', 'set plot nowait', or 'set plot end'\n");

			return FAILURE;
		}

		length3 = strlen( argv[3] );

		if ( length3 <= 1 )
			length3 = 2;

		if ( strncmp( argv[3], "off", length3 ) == 0 ) {

			mx_set_plot_enable( motor_record_list, MXPF_PLOT_OFF );

		} else if ( strncmp( argv[3], "on", length3 ) == 0 ) {

			mx_set_plot_enable( motor_record_list, MXPF_PLOT_ON );

		} else if ( strncmp( argv[3], "nowait", length3 ) == 0 ) {

			mx_set_plot_enable( motor_record_list,
							MXPF_PLOT_NOWAIT );

		} else if ( strncmp( argv[3], "end", length3 ) == 0 ) {

			mx_set_plot_enable( motor_record_list, MXPF_PLOT_END );

		} else {
			fprintf( output,
			"%s: Illegal argument '%s'\n", cname, argv[3]);

			fprintf( output,
"Usage: 'set plot on', 'set plot off', 'set plot nowait', or 'set plot end'\n");

			return FAILURE;
		}

	/* SET OVERWRITE function. */

	} else if ( strncmp( argv[2], "overwrite", length2 ) == 0 ) {

		if ( argc <= 3 ) {
			fprintf(output,
		"Usage: 'set overwrite on' or 'set overwrite off'\n");

			return FAILURE;
		}

		length3 = strlen( argv[3] );

		if ( length3 <= 1 )
			length3 = 2;

		if ( strncmp( argv[3], "off", length3 ) == 0 ) {

			motor_overwrite_on = FALSE;

		} else if ( strncmp( argv[3], "on", length3 ) == 0 ) {

			motor_overwrite_on = TRUE;

		} else {
			fprintf( output,
			"%s: Illegal argument '%s'\n", cname, argv[3]);

			fprintf( output,
		"Usage: 'set overwrite on' or 'set overwrite off'\n");

			return FAILURE;
		}

	/* SET SCANLOG function. */

	} else if ( strncmp( argv[2], "scanlog", length2 ) == 0 ) {

		if ( argc <= 3 ) {
			fprintf(output,
		"Usage: 'set scanlog on' or 'set scanlog off'\n");

			return FAILURE;
		}

		length3 = strlen( argv[3] );

		if ( length3 <= 1 )
			length3 = 2;

		if ( strncmp( argv[3], "off", length3 ) == 0 ) {

			mx_set_scanlog_enable( FALSE );

		} else if ( strncmp( argv[3], "on", length3 ) == 0 ) {

			mx_set_scanlog_enable( TRUE );

		} else {
			fprintf( output,
			"%s: Illegal argument '%s'\n", cname, argv[3]);

			fprintf( output,
		"Usage: 'set scanlog on' or 'set scanlog off'\n");

			return FAILURE;
		}

	/* SET AUTOSAVE function. */

	} else if ( strncmp( argv[2], "autosave", length2 ) == 0 ) {

		if ( argc <= 3 ) {
			fprintf(output,
		"Usage: 'set autosave on' or 'set autosave off'\n");

			return FAILURE;
		}

		if ( allow_scan_database_updates == FALSE ) {
			fprintf( output,
"\n"
"%s: Motor's scan database was specified on the command line with a -S flag.\n"
"     This causes Motor to treat the scan database file as read-only which\n"
"     prevents autosaving.  Thus, autosaving is forced to be off and cannot\n"
"     be changed.\n"
"\n"
"     Invoke motor with a -s flag instead to allow writing.\n"
"\n", cname );

			motor_autosave_on = FALSE;

			return FAILURE;
		}
		length3 = strlen( argv[3] );

		if ( length3 <= 1 )
			length3 = 2;

		if ( strncmp( argv[3], "off", length3 ) == 0 ) {

			motor_autosave_on = FALSE;

		} else if ( strncmp( argv[3], "on", length3 ) == 0 ) {

			motor_autosave_on = TRUE;

		} else {
			fprintf( output,
			"%s: Illegal argument '%s'\n", cname, argv[3]);

			fprintf( output,
		"Usage: 'set autosave on' or 'set autosave off'\n");

			return FAILURE;
		}

	/* SET PARAMETER_WARNING_FLAG function. */

	/* You are not allowed to abbreviate this particular command. */

	} else if ( strcmp( argv[2], "parameter_warning_flag" ) == 0 ) {

		if ( argc <= 3 ) {
			fprintf(output,
			"Usage: 'set parameter_warning_flag [value]\n");

			return FAILURE;
		}

		num_items = sscanf( argv[3], "%d", &int_value );

		if ( num_items != 1 ) {
			fprintf( output,
			"%s: Illegal argument '%s'\n", cname, argv[3]);

			return FAILURE;
		}

		motor_parameter_warning_flag = int_value;

	/* SET BYPASS_LIMIT_SWITCH function. */

	} else if ( strncmp( argv[2], "bypass_limit_switch", length2 ) == 0 ) {

		if ( argc <= 3 ) {
			fprintf( output,
"Usage: 'set bypass_limit_switch on' or 'set bypass_limit_switch off'\n");
			return FAILURE;
		}

		length3 = strlen( argv[3] );

		if ( length3 <= 1 )
			length3 = 2;

		if ( strncmp( argv[3], "off", length3 ) == 0 ) {

			motor_bypass_limit_switch = FALSE;

		} else if ( strncmp( argv[3], "on", length3 ) == 0 ) {

			motor_bypass_limit_switch = TRUE;

		} else {
			fprintf( output,
			"%s: Illegal argument '%s'\n", cname, argv[3]);

			fprintf( output,
"Usage: 'set bypass_limit_switch on' or 'set bypass_limit_switch off'\n");
			return FAILURE;
		}

	/* SET HEADER_PROMPT function. */

	} else if ( strncmp( argv[2], "header_prompt", length2 ) == 0 ) {

		if ( argc <= 3 ) {
			fprintf(output,
		"Usage: 'set header_prompt on' or 'set header_prompt off'\n");

			return FAILURE;
		}

		length3 = strlen( argv[3] );

		if ( length3 <= 1 )
			length3 = 2;

		if ( strncmp( argv[3], "off", length3 ) == 0 ) {

			motor_header_prompt_on = FALSE;

		} else if ( strncmp( argv[3], "on", length3 ) == 0 ) {

			motor_header_prompt_on = TRUE;

		} else {
			fprintf( output,
			"%s: Illegal argument '%s'\n", cname, argv[3]);

			fprintf( output,
		"Usage: 'set header_prompt on' or 'set header_prompt off'\n");

			return FAILURE;
		}

	/* SET DEBUG function. */

	} else if ( strncmp( argv[2], "debug", length2 ) == 0 ) {

		if ( argc <= 3 ) {
			fprintf(output,
			"Usage: 'set debug debug-value'\n");

			return FAILURE;
		}

		num_items = sscanf( argv[3], "%d", &debug_level );

		if ( num_items != 1 ) {
			fprintf( output,
		"%s: Argument '%s' is not an integer number.\n\n"
		"Usage: 'set debug debug-value'\n", cname, argv[3] );

			return FAILURE;
		}

		mx_set_debug_level( debug_level );

	/* SET NETDEBUG function. */

	} else if ( strncmp( argv[2], "netdebug", length2 ) == 0 ) {

		if ( argc <= 3 ) {
			fprintf(output,
			"Usage: 'set netdebug network-debug-value'\n");

			return FAILURE;
		}

		if ( strcmp( argv[3], "on" ) == 0 ) {
			do_netdebug = TRUE;
		} else
		if ( strcmp( argv[3], "off" ) == 0 ) {
			do_netdebug = FALSE;
		} else {
			num_items = sscanf( argv[3], "%lx", &do_netdebug );

			if ( num_items != 1 ) {
				fprintf( output,
		"%s: Argument '%s' is not an integer number.\n\n"
		"Usage: 'set netdebug network-debug-value'\n", cname, argv[3] );

				return FAILURE;
			}
		}

		if ( do_netdebug ) {
			mx_multi_set_debug_flags( motor_record_list,
				(  MXF_NETWORK_SERVER_DEBUG_SUMMARY \
				 | MXF_NETWORK_SERVER_DEBUG_MESSAGE_IDS) );
		} else {
			mx_multi_set_debug_flags( motor_record_list, 0 );
		}

	/* SET ESTIMATE function. */

	} else if ( strncmp( argv[2], "estimate", length2 ) == 0 ) {

		if ( strcmp( argv[3], "on" ) == 0 ) {
			motor_estimate_on = TRUE;
		} else
		if ( strcmp( argv[3], "off" ) == 0 ) {
			motor_estimate_on = FALSE;
		} else {
			fprintf( output, "Usage: ' set estimate on|off'\n" );
			return FAILURE;
		}

	} else {
		fprintf(output,"%s: Unrecognized option '%s'\n\n",
							cname, argv[2]);
		return FAILURE;
	}

	return SUCCESS;
}

int
motor_set_amplifier( MX_RECORD *amplifier_record, int argc, char *argv[] )
{
	static const char cname[] = "set amplifier";

	MX_AMPLIFIER *amplifier;
	double double_value;
	size_t length0;
	mx_status_type mx_status;

	if ( argc <= 1 ) {
		return FAILURE;
	}

	if ( amplifier_record == (MX_RECORD *) NULL ) {
		fprintf( output,
		"%s: amplifier_record pointer passed is NULL.\n", cname );

		return FAILURE;
	}

	if ( amplifier_record->mx_class != MXC_AMPLIFIER ) {
		fprintf( output,
		"%s: The record named '%s' is not a amplifier record.\n",
			cname, amplifier_record->name );

		return FAILURE;
	}

	amplifier = (MX_AMPLIFIER *)(amplifier_record->record_class_struct);

	if ( amplifier == (MX_AMPLIFIER *) NULL ) {
		fprintf( output,
"Internal error in '%s': MX_AMPLIFIER pointer for record '%s' is NULL.\n",
			cname, amplifier_record->name );

		return FAILURE;
	}

#if 0
	fprintf( output,
	"amplifier_record = 0x%p, amplifier_record->name = 0x%p, amplifier = 0x%p\n",
			amplifier_record, amplifier_record->name, amplifier );
	fprintf( output, "Amplifier name = '%s'\n", amplifier_record->name );
	fprintf( output, "argc = %d, argv[0] = '%s'\n", argc, argv[0] );
	fprintf( output, "argv[1] = '%s'\n", argv[1] );
#endif

	/* Figure out which command was requested and execute it. */

	length0 = strlen( argv[0] );

	if ( length0 == 0 )
		length0 = 1;

	if ( strncmp( argv[0], "gain", length0 ) == 0 ) {

		double_value = atof( argv[1] );

		mx_status = mx_amplifier_set_gain( amplifier_record,
							double_value );

		if ( mx_status.code != MXE_SUCCESS ) {
			return FAILURE;
		} else {
			return SUCCESS;
		}

	} else if ( strncmp( argv[0], "sensitivity", length0 ) == 0 ) {

		double_value = mx_divide_safely( 1.0, atof( argv[1] ) );

		mx_status = mx_amplifier_set_gain( amplifier_record,
							double_value );

		if ( mx_status.code != MXE_SUCCESS ) {
			return FAILURE;
		} else {
			return SUCCESS;
		}

	} else if ( strncmp( argv[0], "offset", length0 ) == 0 ) {

		double_value = atof( argv[1] );

		mx_status = mx_amplifier_set_offset( amplifier_record,
							double_value );

		if ( mx_status.code != MXE_SUCCESS ) {
			return FAILURE;
		} else {
			return SUCCESS;
		}

	} else if ( strncmp( argv[0], "time_constant", length0 ) == 0 ) {

		double_value = atof( argv[1] );

		mx_status = mx_amplifier_set_time_constant( amplifier_record,
							double_value );

		if ( mx_status.code != MXE_SUCCESS ) {
			return FAILURE;
		} else {
			return SUCCESS;
		}
	} else {
		fprintf( output, "Unrecognized 'set amplifier' parameter.\n" );

		return FAILURE;
	}
}

