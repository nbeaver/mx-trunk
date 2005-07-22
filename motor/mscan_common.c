/*
 * Name:    mscan_common.c
 *
 * Purpose: Motor scan common prompt routines.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 1999-2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "motor.h"
#include "mdialog.h"
#include "mx_constants.h"
#include "mx_array.h"
#include "mx_scan_linear.h"
#include "mx_measurement.h"
#include "m_time.h"
#include "m_count.h"

int
motor_setup_scan_motor_names(
	char *record_description_buffer,
	size_t record_description_buffer_length,
	long old_scan_num_motors,
	long scan_num_motors,
	MX_RECORD **motor_record_array,
	char **motor_name_array )
{
	MX_RECORD *record, *current_record, *list_head;
	static char buffer[ MXU_RECORD_DESCRIPTION_LENGTH + 1 ];
	char prompt[100];
	size_t buffer_left;
	char *default_string_ptr;
	char *name;
	int i, status, string_length, valid_motor_name;

	fprintf( output, "Type ? below to get a list of motor names.\n" );

	for ( i = 0; i < scan_num_motors ; i++ ) {

		if ( scan_num_motors == 1 ) {
			sprintf(prompt, "Enter motor name -> ");
		} else {
			sprintf(prompt, "Enter motor(%d) name -> ", i);
		}

		valid_motor_name = FALSE;

		while ( ! valid_motor_name ) {
			string_length = sizeof(buffer)-1;

			if ( motor_record_array == NULL ) {
				default_string_ptr = NULL;
			} else {
				if ( i < old_scan_num_motors ) {
					name = motor_record_array[i]->name;

					if ( mx_get_record( motor_record_list,
						    name ) != NULL )
					{
						default_string_ptr = name;
					} else {
						mx_warning(
				    "Motor '%s' does not exist.  Skipping...",
				    			name );

						default_string_ptr = NULL;
					}
				} else {
					default_string_ptr = NULL;
				}
			}

			status = motor_get_string( output, prompt,
					default_string_ptr,
					&string_length, buffer);

			if ( status != SUCCESS )
				return status;

			if ( string_length <= 0 ) {
				fprintf( output,
				"Error: No motor name was entered.\n" );

			} else if ( buffer[0] == '?' ) {

				/* The first character the user typed
				 * was a question mark.
				 */

				list_head = motor_record_list->list_head;

				current_record = list_head->next_record;

				while ( current_record != list_head ) {
					if ((current_record->mx_superclass
								== MXR_DEVICE)
					  &&(current_record->mx_class
								 == MXC_MOTOR))
					{
					    fprintf( output, "%s ",
						current_record->name );
					}
					current_record
						= current_record->next_record;
				}
				fprintf( output, "\n" );
			} else {
				/* Is this name really the name of a motor? */

				record = mx_get_record(
						motor_record_list, buffer );

				if ( record != (MX_RECORD *) NULL ) {
					if (
					   (record->mx_superclass == MXR_DEVICE)
					  && (record->mx_class == MXC_MOTOR) )
					{
					    strcpy( motor_name_array[i], "" );

					    strncat( motor_name_array[i],
					      buffer, MXU_RECORD_NAME_LENGTH);

					    valid_motor_name =TRUE;
					}
				}
				if ( ! valid_motor_name ) {
					fprintf(output,
				"'%s' is not the name of a known motor.\n",
						buffer);
				}
			}
		}
	}

	for ( i = 0; i < scan_num_motors; i++ ) {
		string_length = strlen(record_description_buffer);
		buffer_left = record_description_buffer_length - string_length;

		strncat( record_description_buffer,
				motor_name_array[i], buffer_left );

		string_length = strlen(record_description_buffer);
		buffer_left = record_description_buffer_length - string_length;

		strncat( record_description_buffer, " ", buffer_left );
	}

	return SUCCESS;
}

int
motor_setup_input_devices(
	MX_SCAN *old_scan,
	long scan_class,
	long scan_type,
	char *input_devices_buffer,
	size_t input_devices_buffer_length,
	MX_RECORD **first_input_device_record )
{
	MX_RECORD *record, *current_record, *list_head;
	MX_RECORD **input_device_array;
	static char buffer[ MXU_RECORD_DESCRIPTION_LENGTH + 1 ];
	char prompt[100];
	char *old_input_device_name_ptr;
	char *name;
	int i, status;
	int string_length;
	size_t buffer_left;
	int valid_input_device_name;
	long scan_num_input_devices;

	long default_long;

	char **input_name_array;
	long input_name_dimension_array[2];
	
	static size_t name_element_size_array[2] = {
		sizeof(char), sizeof(char *) };

	record = NULL;

	if ( first_input_device_record != NULL ) {
		*first_input_device_record = NULL;
	}

	/* Set up the input devices. */

	if ( old_scan == NULL ) {
		default_long = 1;
	} else {
		default_long = old_scan->num_input_devices;
	}

	status = motor_get_long( output,
			"Enter number of scalers or other input devices -> ",
			TRUE, default_long,
			&scan_num_input_devices, 0, 1000 );

	if ( status == FAILURE ) {
		return status;
	}

	/* Set up the input device array. */

	if ( scan_num_input_devices <= 0 ) {
		input_name_array = NULL;
	} else {
		input_name_dimension_array[0] = scan_num_input_devices;
		input_name_dimension_array[1] = MXU_RECORD_NAME_LENGTH + 1;

		input_name_array = (char **) mx_allocate_array(
		    2, input_name_dimension_array, name_element_size_array );

		if ( input_name_array == (char **) NULL ) {
			fprintf( output,
		    "Unable to allocate memory for list of input devices.\n");
			return FAILURE;
		}
	}

	if ( scan_num_input_devices <= 0 ) {
		input_device_array = NULL;
	} else {
		if ( old_scan == (MX_SCAN *) NULL ) {
			input_device_array = NULL;
		} else {
			input_device_array = old_scan->input_device_array;
		}
	}

	fprintf( output,
		"Type ? below to get a list of valid input devices.\n" );

	for ( i = 0; i < scan_num_input_devices; i++ ) {

		if ( scan_num_input_devices == 1 ) {
			sprintf(prompt, "Enter input device name -> ");
		} else {
			sprintf(prompt, "Enter input device(%d) name -> ", i);
		}

		valid_input_device_name = FALSE;

		while ( ! valid_input_device_name ) {
			string_length = sizeof(buffer)-1;

			if ( old_scan == NULL ) {
				old_input_device_name_ptr = NULL;
			} else {
				if ( i >= old_scan->num_input_devices ) {
					old_input_device_name_ptr = NULL;

				} else if ( input_device_array == NULL ) {
					old_input_device_name_ptr = NULL;

				} else {
					name = input_device_array[i]->name;

					if ( mx_get_record( motor_record_list,
							name ) != NULL )
					{
						old_input_device_name_ptr
							= name;
					} else {
						mx_warning(
			    "Input device '%s' does not exist.  Skipping...",
				    			name );

						old_input_device_name_ptr
							= NULL;
					}
				}
			}

			status = motor_get_string( output, prompt,
					old_input_device_name_ptr,
					&string_length, buffer);

			if ( status != SUCCESS ) {
				if ( input_name_array != NULL ) {
					(void) mx_free_array( input_name_array,
						2, input_name_dimension_array,
						name_element_size_array );
				}
				return status;
			}

			if ( string_length <= 0 ) {
			    fprintf( output,
				"Error: No input device name was entered.\n" );

			} else if ( buffer[0] == '?' ) {

			    /* The first character the user typed
			     * was a question mark.
			     */

			    list_head = motor_record_list->list_head;

			    current_record = list_head->next_record;

			    while ( current_record != list_head ) {
				if ( (scan_type == MXS_QUI_MCS)
				  || (scan_type == MXS_QUI_APS_ID) )
				{
				    if ( current_record->mx_type
						== MXT_SCL_MCS )
				    {
					fprintf( output, "%s ",
						current_record->name );
				    }
				} else {
				    switch ( current_record->mx_superclass ) {
				    case MXR_VARIABLE:
					fprintf( output, "%s ",
						current_record->name );
					break;
				    case MXR_DEVICE:
					switch( current_record->mx_class ) {
					case MXC_ANALOG_INPUT:
					case MXC_ANALOG_OUTPUT:
					case MXC_DIGITAL_INPUT:
					case MXC_DIGITAL_OUTPUT:
					case MXC_MOTOR:
					case MXC_SCALER:
					case MXC_TIMER:
					case MXC_AMPLIFIER:
					case MXC_MULTICHANNEL_ANALYZER:
					    fprintf( output, "%s ",
						current_record->name );
					    break;
					}
					break;
				    }
				}
				current_record = current_record->next_record;
			    }
			    fprintf( output, "\n" );
			} else {
			    /* Does this name really match a valid 
			     * input device?
			     */

			    record = mx_get_record( motor_record_list, buffer );

			    if ( record != (MX_RECORD *) NULL ) {
				if ( (scan_type == MXS_QUI_MCS)
				  || (scan_type == MXS_QUI_APS_ID) )
				{
				    if ( record->mx_class == MXC_SCALER ) {
					mx_strncpy( input_name_array[i],
					    buffer,MXU_RECORD_NAME_LENGTH + 1);

					valid_input_device_name = TRUE;
				    }
				} else {
				    switch ( record->mx_superclass ) {
				    case MXR_VARIABLE:
					mx_strncpy( input_name_array[i],
					    buffer,MXU_RECORD_NAME_LENGTH + 1);

					valid_input_device_name = TRUE;
					break;
				    case MXR_DEVICE:
					switch( record->mx_class ) {
					case MXC_ANALOG_INPUT:
					case MXC_ANALOG_OUTPUT:
					case MXC_DIGITAL_INPUT:
					case MXC_DIGITAL_OUTPUT:
					case MXC_MOTOR:
					case MXC_SCALER:
					case MXC_TIMER:
					case MXC_RELAY:
					case MXC_AMPLIFIER:
					case MXC_MULTICHANNEL_ANALYZER:
					    mx_strncpy( input_name_array[i],
					      buffer,MXU_RECORD_NAME_LENGTH+1);

					    valid_input_device_name = TRUE;
					    break;
					}
					break;
				    }
				}
			    }
			    if ( ! valid_input_device_name ) {
				fprintf(output,
			"'%s' is not the name of a valid input device.\n",
					buffer);
			    }
			}
		}
		if ( i == 0 ) {
			if ( first_input_device_record != NULL ) {
				*first_input_device_record = record;
			}
		}
	}

	/* Now format this part of the record description. */

	sprintf( input_devices_buffer, "%ld ", scan_num_input_devices );

	for ( i = 0; i < scan_num_input_devices; i++ ) {
		string_length = strlen(input_devices_buffer);
		buffer_left = input_devices_buffer_length - string_length;

		strncat( input_devices_buffer,
				input_name_array[i], buffer_left );

		string_length = strlen(input_devices_buffer);
		buffer_left = input_devices_buffer_length - string_length;

		strncat( input_devices_buffer, " ", buffer_left );
	}

	if ( input_name_array != NULL ) {
		(void) mx_free_array( input_name_array,
			2, input_name_dimension_array,
			name_element_size_array );
	}

#if 0
	fprintf( output, "*** input_devices_buffer = '%s'\n\n",
				input_devices_buffer );
#endif

	return SUCCESS;
}

int
motor_setup_measurement_parameters(
	MX_SCAN *old_scan,
	char *measurement_parameters_buffer,
	size_t measurement_parameters_buffer_length,
	int setup_measurement_interval )
{
	double scan_settling_time, default_double;
	long scan_measurement_type, default_long;
	size_t string_length, remaining_length;
	char *ptr;
	int status;

	if ( old_scan == NULL ) {
		default_double = 0.0;
	} else {
		default_double = old_scan->settling_time;
	}

	status = motor_get_double( output,
			"Enter settling time in seconds -> ",
			TRUE, default_double,
			&scan_settling_time, 0.0, 1.0e30 );

	if ( status != SUCCESS ) {
		return status;
	}

	if ( old_scan == NULL ) {
		default_long = MXM_PRESET_TIME;
	} else {
		default_long = old_scan->measurement.type;
	}

	fprintf( output,
		"Select measurement type: \n"
		"  1.  Preset time\n"
		"  2.  Preset count\n"
		"\n" );

	status = motor_get_long( output,
		"--> ", TRUE, default_long,
		&scan_measurement_type, 0, 2 );

	if ( status != SUCCESS ) {
		return status;
	}

	switch( scan_measurement_type ) {
	case MXM_NONE:
		sprintf( measurement_parameters_buffer, "%g none ",
						scan_settling_time );
	case MXM_PRESET_TIME:
		sprintf( measurement_parameters_buffer, "%g preset_time ",
						scan_settling_time );
		break;
	case MXM_PRESET_COUNT:
		sprintf( measurement_parameters_buffer, "%g preset_count ",
						scan_settling_time );
		break;
	}

	string_length = strlen( measurement_parameters_buffer );

	ptr = measurement_parameters_buffer + string_length;

	remaining_length = measurement_parameters_buffer_length - string_length;

	switch( scan_measurement_type ) {
	case MXM_NONE:
		mx_strncpy( ptr, "\"\" ", remaining_length );

		status = SUCCESS;
		break;

	case MXM_PRESET_TIME:
		status = motor_setup_preset_time_measurement( old_scan,
					ptr, remaining_length,
					setup_measurement_interval );
		break;

	case MXM_PRESET_COUNT:
		status = motor_setup_preset_count_measurement( old_scan,
					ptr, remaining_length,
					setup_measurement_interval );
		break;

	default:
		fprintf( output, "Error: unsupported measurement type %ld.\n",
					scan_measurement_type );

		status = FAILURE;
		break;
	}

	if ( status != SUCCESS ) {
		return status;
	}

#if 0
	fprintf( output, "*** measurement_parameters_buffer = '%s'\n",
				measurement_parameters_buffer );
#endif

	return SUCCESS;
}

int
motor_setup_preset_time_measurement(
	MX_SCAN *old_scan,
	char *parameter_buffer,
	size_t parameter_buffer_length,
	int setup_measurement_interval )
{
	MX_RECORD *timer_record;
	double scan_integration_time, default_double;
	char old_timer_name[ MXU_RECORD_NAME_LENGTH + 1 ];
	char buffer[ MXU_RECORD_NAME_LENGTH + 1 ];
	char format_buffer[20];
	char *default_string;
	int string_length;
	char default_timer_name[] = "timer1";
	int status, num_items;

	if ( old_scan == NULL ) {
		default_double = 0.0;
		default_string = default_timer_name;
	} else {
		if ( old_scan->measurement.type != MXM_PRESET_TIME ) {
			default_double = 0.0;
			default_string = default_timer_name;
		} else {
			sprintf( format_buffer, "%%lg %%%ds",
						MXU_RECORD_NAME_LENGTH );

			num_items = sscanf(
				old_scan->measurement.measurement_arguments,
				format_buffer,
				&scan_integration_time,
				old_timer_name );

			if ( num_items != 2 ) {
				fprintf( output,
			"Error: Can't parse old measurement arguments '%s'.\n",
				old_scan->measurement.measurement_arguments );

				return FAILURE;
			}
			default_double = scan_integration_time;
			default_string = old_timer_name;
		}
	}

	if ( setup_measurement_interval == FALSE ) {
		scan_integration_time = 0.0;
	} else {
		status = motor_get_double( output,
			"Enter measurement time in seconds -> ",
			TRUE, default_double,
			&scan_integration_time, 0.0, 1.0e30 );

		if ( status != SUCCESS )
			return status;
	}

	string_length = sizeof(buffer) - 1;

	status = motor_get_string( output,
			"Enter timer name -> ",
			default_string, &string_length, buffer );
	
	if ( status != SUCCESS )
		return status;

	/* Does the requested timer exist? */

	timer_record = mx_get_record( motor_record_list, buffer );

	if ( timer_record == NULL ) {
		fprintf( output,
			"Error: The timer '%s' does not exist.\n", buffer );

		return FAILURE;
	}

	if ( ( timer_record->mx_superclass != MXR_DEVICE )
	  || ( timer_record->mx_class != MXC_TIMER ) )
	{
		fprintf( output,
			"Error: the record '%s' is not a timer record.\n",
				timer_record->name );

		return FAILURE;
	}

	sprintf( parameter_buffer, "\"%g %s\" ",
			scan_integration_time, timer_record->name );

	return SUCCESS;
}

int
motor_setup_preset_count_measurement(
	MX_SCAN *old_scan,
	char *parameter_buffer,
	size_t parameter_buffer_length,
	int setup_measurement_interval )
{
	MX_RECORD *scaler_record;
	long scaler_preset_count, default_long;
	char old_scaler_name[ MXU_RECORD_NAME_LENGTH + 1 ];
	char buffer[ MXU_RECORD_NAME_LENGTH + 1 ];
	char format_buffer[20];
	char *default_string;
	int string_length;
	int status, num_items;

	if ( old_scan == NULL ) {
		default_long = 0;
		default_string = NULL;
	} else {
		if ( old_scan->measurement.type != MXM_PRESET_COUNT ) {
			default_long = 0;
			default_string = NULL;
		} else {
			sprintf( format_buffer, "%%ld %%%ds",
						MXU_RECORD_NAME_LENGTH );

			num_items = sscanf(
				old_scan->measurement.measurement_arguments,
				format_buffer,
				&scaler_preset_count,
				old_scaler_name );

			if ( num_items != 2 ) {
				fprintf( output,
			"Error: Can't parse old measurement arguments '%s'.\n",
				old_scan->measurement.measurement_arguments );

				return FAILURE;
			}
			default_long = scaler_preset_count;
			default_string = old_scaler_name;
		}
	}

	if ( setup_measurement_interval == FALSE ) {
		scaler_preset_count = 0;
	} else {
		status = motor_get_long( output,
			"Enter preset count for scaler -> ",
			TRUE, default_long,
			&scaler_preset_count, 0, MX_LONG_MAX );

		if ( status != SUCCESS )
			return status;
	}

	string_length = sizeof(buffer) - 1;

	status = motor_get_string( output,
			"Enter scaler name for preset count channel -> ",
			default_string, &string_length, buffer );

	if ( status != SUCCESS )
		return status;

	/* Does the requested scaler exist? */

	scaler_record = mx_get_record( motor_record_list, buffer );

	if ( scaler_record == NULL ) {
		fprintf( output,
			"Error: The scaler '%s' does not exist.\n", buffer );

		return FAILURE;
	}

	if ( ( scaler_record->mx_superclass != MXR_DEVICE )
	  || ( scaler_record->mx_class != MXC_SCALER ) )
	{
		fprintf( output,
			"Error: The record '%s' is not a scaler record.\n",
				scaler_record->name );

		return FAILURE;
	}

	sprintf( parameter_buffer, "\"%ld %s\" ",
			scaler_preset_count, scaler_record->name );

	return SUCCESS;
}

int
motor_setup_datafile_and_plot_parameters(
	MX_SCAN *old_scan,
	long scan_class,
	long scan_type,
	char *datafile_and_plot_buffer,
	size_t datafile_and_plot_buffer_length )
{
	MX_DATAFILE *datafile;
	MX_PLOT *plot;
	int datafile_type_name_length;
	char datafile_type_name[MXU_DATAFILE_TYPE_NAME_LENGTH + 1];
	char datafile_options[MXU_DATAFILE_OPTIONS_LENGTH + 1];
	char default_datafile_options[MXU_DATAFILE_OPTIONS_LENGTH + 1];
	int plot_type_name_length;
	char plot_type_name[MXU_PLOT_TYPE_NAME_LENGTH + 1];
	char plot_options[MXU_PLOT_OPTIONS_LENGTH + 1];
	char default_plot_options[MXU_PLOT_OPTIONS_LENGTH + 1];
	int plot_arguments_length;
	char plot_arguments[MXU_PLOT_ARGUMENTS_LENGTH + 1];
	int output_filename_length;
	char output_filename[MXU_FILENAME_LENGTH + 1];
	int status, use_plot_arguments;
	int i, valid_input, string_length;
	char *ptr, *default_string_ptr;

	int default_file_type_number;

	static char default_plot_arguments_string[] = "$f[0]";

	static char *valid_file_types[]        = {"text","sff","xafs"};
	static int file_type_name_min_length[] = { 1,     1,    1    };
	static int num_file_types = sizeof( file_type_name_min_length )
				/ sizeof( file_type_name_min_length[0] );

	int default_plot_type_number;

	static char *valid_plot_types[]        = {"gnuplot","gnuxafs","none"};
	static int plot_type_name_min_length[] = { 4,        4,        1    };
	static int num_plot_types = sizeof( plot_type_name_min_length )
				/ sizeof( plot_type_name_min_length[0] );

	/* Get the output filename. */

	if ( old_scan == (MX_SCAN *) NULL ) {
		datafile = NULL;
	} else {
		datafile = &(old_scan->datafile);
	}

	if ( datafile == NULL ) {
		default_string_ptr = NULL;
	} else {
		default_string_ptr = datafile->filename;
	}

	valid_input = FALSE;

	while ( valid_input == FALSE ) {
		output_filename_length = sizeof(output_filename)-1;

		status = motor_get_string( output, "Enter output filename -> ",
				default_string_ptr,
				&output_filename_length, output_filename );

		if ( status == FAILURE ) {
			return status;
		}

		/* Did the user give us a zero length filename? */

		if ( output_filename_length <= 0 ) {
			fprintf( output,
	"Error: Zero length output filenames are not allowed.\n");

			/* Go back to the top of the while() loop. */

			continue;
		}

		/* Are there any characters embedded in the file name
		 * that we cannot deal with?
		 */

		ptr = strpbrk( output_filename, MX_ILLEGAL_FILENAME_CHARS );

		if ( ptr == NULL ) {
			valid_input = TRUE;
		} else {
			switch( *ptr ) {
			case ' ':
				fprintf( output,
	"Error: Blank spaces are not allowed in an output filename.\n");
				break;
			case '\t':
				fprintf( output,
	"Error: Tab characters are not allowed in an output filename.\n");
				break;
			case '\n':
				fprintf( output,
	"Error: Newline characters are not allowed in an output filename.\n");
				break;
			case '\"':
				fprintf( output,
"Error: Double quote characters are not allowed in an output filename.\n");
				break;
			}
		}
	}

	/* Get the datafile type. */

	datafile_type_name_length = sizeof(datafile_type_name)-1;

	if ( scan_class == MXS_XAFS_SCAN ) {
		default_file_type_number = 2;
	} else {
		default_file_type_number = 1;
	}

	if ( datafile != NULL ) {
		for ( i = 0; i < num_file_types; i++ ) {
			if ( strcmp( datafile->typename,
						valid_file_types[i] ) == 0 ) {

				default_file_type_number = i;

				break;       /* Exit the for() loop. */
			}
		}
	}

	status = motor_get_string_from_list( output,
			"Enter datafile type ",
			num_file_types,
			file_type_name_min_length,
			valid_file_types,
			default_file_type_number,
			&datafile_type_name_length,
			datafile_type_name );

	if ( status == FAILURE ) {
		return status;
	}

	fprintf( output, "Selected file type is '%s'.\n", datafile_type_name );

	/* Get datafile options. */

	if ( old_scan == (MX_SCAN *) NULL ) {
		strcpy( default_datafile_options, "" );
	} else {
		mx_strncpy( default_datafile_options,
			old_scan->datafile.options,
			MXU_DATAFILE_OPTIONS_LENGTH );
	}

	string_length = MXU_DATAFILE_OPTIONS_LENGTH;

	status = motor_get_string( output,
		"Select optional datafile features -> ",
		default_datafile_options,
		&string_length, datafile_options );

	if ( status == FAILURE ) {
		return status;
	}

	/* Get the plot type. */

	if ( old_scan == (MX_SCAN *) NULL ) {
		plot = NULL;
	} else {
		plot = &(old_scan->plot);
	}

	plot_type_name_length = sizeof(plot_type_name)-1;

	if ( scan_class == MXS_XAFS_SCAN ) {
		default_plot_type_number = 1;
	} else {
		default_plot_type_number = 0;
	}

	if ( plot != NULL ) {
		for ( i = 0; i < num_plot_types; i++ ) {
			if ( strcmp( plot->typename,
						valid_plot_types[i] ) == 0 ) {

				default_plot_type_number = i;

				break;       /* Exit the for() loop. */
			}
		}
	}

	status = motor_get_string_from_list( output,
			"Enter plot type ",
			num_plot_types,
			plot_type_name_min_length,
			valid_plot_types,
			default_plot_type_number,
			&plot_type_name_length,
			plot_type_name );

	if ( status == FAILURE ) {
		return status;
	}

	fprintf( output, "Selected plot type is '%s'.\n", plot_type_name );

	/* Get plot options. */

	if ( old_scan == (MX_SCAN *) NULL ) {
		strcpy( default_plot_options, "" );
	} else {
		mx_strncpy( default_plot_options,
			old_scan->plot.options,
			MXU_PLOT_OPTIONS_LENGTH );
	}

	string_length = MXU_PLOT_OPTIONS_LENGTH;

	status = motor_get_string( output,
		"Select optional plot features -> ",
		default_plot_options,
		&string_length, plot_options );

	if ( status == FAILURE ) {
		return status;
	}

	/* Getting the plot arguments is more complicated than most things. */

#if 1

	use_plot_arguments = TRUE;

#else

	if ( strcmp( plot_type_name, "gnuplot" ) == 0 ) {
		use_plot_arguments = FALSE;
	} else {
		use_plot_arguments = TRUE;
	}

#endif

	if (use_plot_arguments == FALSE ) {
		strcpy( plot_arguments, "" );
	} else {
		plot_arguments_length = sizeof(plot_arguments)-1;

		if ( plot == NULL || plot->plot_arguments == NULL ) {
			default_string_ptr = default_plot_arguments_string;
		} else {
			default_string_ptr = plot->plot_arguments;
		}

		status = motor_get_string( output,
			"Enter plot arguments separated by semicolons\n  -> ",
			default_string_ptr,
			&plot_arguments_length, plot_arguments );

		if ( plot_arguments_length == 0 ) {
			strcpy( plot_arguments, "$f[0]" );
		}
	}

	if ( strlen( datafile_options ) > 0 ) {
		sprintf( datafile_and_plot_buffer, "%s:%s %s ",
			datafile_type_name, datafile_options, output_filename );
	} else {
		sprintf( datafile_and_plot_buffer, "%s %s ",
			datafile_type_name, output_filename );
	}

	ptr = datafile_and_plot_buffer + strlen( datafile_and_plot_buffer );

	if ( strlen( plot_options ) > 0 ) {
		sprintf( ptr, "%s:%s \"%s\" ",
			plot_type_name, plot_options, plot_arguments );
	} else {
		sprintf( ptr, "%s \"%s\" ",
			plot_type_name, plot_arguments );
	}

#if 0
	fprintf( output, "*** datafile_and_plot_buffer = '%s'\n",
				datafile_and_plot_buffer );
#endif

	return SUCCESS;
}

