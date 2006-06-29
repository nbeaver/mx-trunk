#! /bin/sh
#
#  Name:    example1.tcl
#
#  Purpose: This example performs a simple step scan of a motor
#          using ordinary motor, scaler, and timer calls.
#
#  Author:  William Lavender
#
#-------------------------------------------------------------------------
#
#  Copyright 2000-2001 Illinois Institute of Technology
#
#  See the file "LICENSE" for information on usage and redistribution
#  of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#
#-------------------------------------------------------------------------
#
#  Look for mxtclsh in the users' PATH. \
exec mxtclsh "$0" ${1+"$@"}

#  Define the location of the MX installation directory.

set mxdir "/opt/mx"

#set mxdir [ file join $env(HOME) "mxtest" ]

#  Define the location of the example MX database.

set database_filename "example.dat"

#  Source the Tcl interface to the MX Tcl library.

source [ file join $mxdir "lib" "mx.tcl" ]

#  Load the MX Tcl package.

package require Mx

#  Initialize the MX Tcl library.

if [ catch { ::Mx::init } result ] {
	puts "The MX initialization procedure Mx::init failed."
	puts "The reason given is '$result'.  Exiting..."
	exit 1
}

#  Read in the MX database used by this example.

if [ catch { Mx::setup_database $database_filename } record_list ] {

	puts "Cannot setup the MX database '$database_filename'."
	exit 1
}

#  Find the devices to be used by the scan.

if [ catch { $record_list get_record "energy" } energy_motor_record ] {

	puts "Cannot find the motor 'energy'."
	exit 1
}

if [ catch { $record_list get_record "Io" } i_zero_record ] {

	puts "Cannot find the scaler 'Io'."
	exit 1
}

if [ catch { $record_list get_record "It" } i_trans_record ] {

	puts "Cannot find the scaler 'It'."
	exit 1
}

if [ catch { $record_list get_record "timer1" } timer_record ] {

	puts "Cannot find the timer 'timer1'."
	exit 1
}

#  Setup the scan parameters for a copper K edge scan.

set start 8950			;# in eV
set step_size 1.0		;# in eV
set num_steps 101
set measurement_time 1.0	;# in seconds

#  Perform the scan.

set error 0

puts "Moving to start position.\n"

for { set i 0 } { $i < $num_steps } { incr i } {

	#  Clear the scalers.

	if [ catch { $i_zero_record clear } ] {

		set error 1
		break
	}

	if [ catch { $i_trans_record clear } ] {

		set error 1
		break
	}

	#  Compute the energy for the next step.

	set energy [ expr ( $start + $step_size * $i ) ]

	#  Move to the new energy.

	if [ catch { $energy_motor_record move_absolute $energy } ] {

		set error 1
		break
	}

	#  Wait for the motor to stop moving.

	set busy 1

	while { $busy } {

		after 10	;# Wait for 10 milliseconds

		if [ catch { $energy_motor_record is_busy } busy ] {

			set error 1
			break
		}
	}

	#  Start the measurement timer.

	if [ catch { $timer_record start $measurement_time } ] {

		set error 1
		break
	}

	#  Wait until the timer stops counting.

	set busy 1

	while { $busy } {

		after 10	;# Wait for 10 milliseconds

		if [ catch { $timer_record is_busy } busy ] {

			set error 1
			break
		}
	}

	#  Read out the scalers.

	if [ catch { $i_zero_record read } i_zero_value ] {

		set error 1
		break
	}

	if [ catch { $i_trans_record read } i_trans_value ] {

		set error 1
		break
	}

	#  Print out the values.

	puts [ format "%10.3f  %10lu  %10lu" \
		$energy $i_zero_value $i_trans_value ]
}

if { $error } {
	puts "The scan aborted abnormally."
	exit 1
}

exit

