#! /bin/sh
#
# Name:    example2.c
#
# Purpose: This example performs a simple step scan of a motor
#          by creating and then executing an MX scan record.
#
# Author:  William Lavender
#
#-------------------------------------------------------------------------
#
# Copyright 2000-2001 Illinois Institute of Technology
#
# See the file "LICENSE" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#
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

#  Setup the scan parameters for a copper K edge scan.

set start 8950			;# in eV
set step_size 1.0		;# in eV
set num_steps 101
set measurement_time 1.0	;# in seconds
set settling_time 0.1		;# in seconds

#  Construct a scan description for the requested scan.

set    format_string "example2scan scan linear_scan motor_scan \"\" \"\" "

append format_string "1 1 1 energy 2 Io It 0 %g preset_time \"%g timer1\" "

append format_string "text example2scan.out gnuplot "

append format_string "log(\$f\[0\]/\$f\[1\]) %g %g %d"

set description [ format $format_string \
			$settling_time $measurement_time \
			$start $step_size $num_steps ]

#  Display the scan description.

puts "Scan description = '$description'\n"

#  Add a corresponding scan record to the MX database of the current process.

if [ catch { $record_list create_record_from_description $description } \
     scan_record ] {

	puts "Cannot create the scan record."
	exit 1
}

if [ catch { $scan_record finish_record_initialization } ] {

	puts "Cannot finish initialization of the scan record."
	exit 1
}

#  Perform the scan.

if [ catch { $scan_record perform_scan } ] {

	puts "The scan aborted abnormally."
	exit 1
}

#  Delete the scan record from the client's database.
#
#  Strictly speaking, this is not really necessary since the example
#  program is about to exit anyway.  It is shown primarily to give
#  an example of how scan records are normally deleted in long running
#  application programs.

if [ catch { $scan_record delete_record } ] {

	puts "The attempt to delete the scan record failed."
	exit 1
}

exit

