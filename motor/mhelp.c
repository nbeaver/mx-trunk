/*
 * Name:    mhelp.c
 *
 * Purpose: Help message handling for 'motor'.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2009-2010, 2012-2013 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "motor.h"
#include "mx_key.h"
#include "mx_console.h"

static char msg_help[] =
"A more complete command summary may be found at:\n"
"\n"
"    http://mx.iit.edu/motor/\n"
"\n"
"Single word commands are:\n"
"    exit                           - Exits the motor program, saving the\n"
"                                       motor parameters in 'mxmotor.dat' and\n"
"                                       the scan parameters in 'mxscan.dat'\n"
"    help                           - Displays this message.\n"
"\n"
"    help [...]                     - Displays help for individual commands\n"
"                                       listed below.\n"
"\n"
"    abort ...                      - Stop an action such as a motor move.\n"
"    ad ...                         - Area detector commands.\n"
"    area_detector ...              - Area detector commands.\n"
"    cd 'directory name'            - Change directory to 'directory name'.\n"
"    changer ...                    - Sample changer commands.\n"
"    close [...]                    - Close relay or shutter.\n"
"    copy scan 'scan1' 'scan2'      - Make 'scan2' an exact copy of 'scan1'.\n"
"    count 'seconds' 'scaler1' 'scaler2' ...\n"
"                                   - Count scaler, ADC, or dinput channels.\n"
"    date                           - Shows the current date and time.\n"
"    delete scan 'scan_name'        - Delete scan parameters for scan.\n"
"    gpib ...                       - Communicate with a GPIB device.\n"
"    home 'motorname' 'direction'   - Perform a motor home search \n"
"                                       (if supported).\n"
"    insert [...]                   - Insert shutter.\n"
"    kill ...                       - Abruptly stop an action such as a\n"
"                                       motor move (not recommended).\n"
"    load ...                       - Load motor or scan params from a file.\n"
"    mca ...                        - Multichannel analyzer commands.\n"
"    mcs ...                        - Multichannel scaler commands.\n"
"    measure dark_currents          - Measure dark currents for all scalers\n"
"                                       and selected analog inputs.\n"
"    mjog 'motorname' [step_size]   - Move motor using single keystrokes.\n"
"    modify scan 'scan_name'        - Modify scan params for 'scan_name'.\n"
"    move 'motorname' 'distance'    - Move absolute 'motorname'\n"
"    move 'motorname' steps 'steps' - Move absolute 'motorname' in steps.\n"
"    mrel 'motorname' 'distance'    - Move relative 'motorname'\n"
"    mrel 'motorname' steps 'steps' - Move relative 'motorname' in steps.\n"
"    open [...]                     - Open relay or shutter.\n"
"    ptz ...                        - Pan/tilt/zoom camera base commands.\n"
"    remove [...]                   - Remove shutter.\n"
"    rename scan 'scan1' 'scan2'    - Rename 'scan1' to 'scan2'.\n"
"    resynchronize 'record_name'    - Resynchronizes the MX database with the\n"
"                                       controller for record 'record_name'\n"
"    rs232 ...                      - Communicate with an RS-232 device.\n"
"    sample_changer ...             - Sample changer commands.\n"
"    save ...                       - Save motor or scan params to a file.\n"
"    sca ...                        - Single channel analyzer commands.\n"
"    scan 'scan_name'               - Perform the scan called 'scan_name'.\n"
"    set ...                        - Set motor, device, plot, etc. params.\n"
"    setup scan 'scan_name'         - Prompts for setup of scan parameters.\n"
"    show ...                       - Show motor, device, plot, etc. params.\n"
"                                       Wildcards '*' and '?' may be used to\n"
"                                       list only a subset of records.\n"
"    showall record 'recordname'    - Show all internal record parameters\n"
"    start ...                      - Starts an MCA, MCS, or pulse generator\n"
"    stop ...                       - Stop an action such as a motor move.\n"
"    vinput ...                     - Video input commands.\n"
"    video_input ...                - Video input commands.\n"
"    wvout ...                      - Waveform output commands.\n"
"    ! 'command_name'               - Executes an external command.\n"
"      or\n"
"    $ 'command_name'\n"
"    @ 'script_name'                - Executes a script of motor commands.\n"
"    & 'program_name'               - Executes an external program that\n"
"                                       can directly invoke 'motor' program\n"
"                                       commands.\n";

int
motor_help_fn( int argc, char *argv[] )
{
	int result;

	if ( argc <= 1 ) {
		fprintf( output, "argc has unexpected value of %d\n", argc );
		return FAILURE;
	} else
	if ( argc == 2 ) {
		mx_paged_console_output( output, msg_help );
	} else {
		if ( strcmp( argv[2], "exit" ) == 0 ) {
			fprintf( output,
				"This command exits the motor program.\n" );
			return SUCCESS;
		} else
		if ( strcmp( argv[2], "quit" ) == 0 ) {
			fprintf( output,
				"This command exits the motor program.\n" );
			return SUCCESS;
		} else
		if ( strcmp( argv[2], "close" ) == 0 ) {
			fprintf( output,
	"Closes the specified relay or shutter.\n"
	"If no name is specified, the record name defaults to 'shutter'\n" );
			return SUCCESS;
		} else
		if ( strcmp( argv[2], "open" ) == 0 ) {
			fprintf( output,
	"Opens the specified relay or shutter.\n"
	"If no name is specified, the record name defaults to 'shutter'\n" );
			return SUCCESS;
		} else
		if ( strcmp( argv[2], "insert" ) == 0 ) {
			fprintf( output,
	"Inserts the specified shutter or filter.\n"
	"If no name is specified, the record name defaults to 'shutter'\n" );
			return SUCCESS;
		} else
		if ( strcmp( argv[2], "remove" ) == 0 ) {
			fprintf( output,
	"Removes the specified shutter or filter.\n"
	"If no name is specified, the record name defaults to 'shutter'\n" );
			return SUCCESS;
		} else
		if ( strcmp( argv[2], "show" ) == 0 ) {
			result = cmd_run_command( "show help" );
		} else {
			result = cmd_run_command( argv[2] );
		}

		return result;
	}

	return SUCCESS;
}

