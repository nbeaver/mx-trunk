#! /usr/bin/env python
#
#  Name:    example1.py
#
#  Purpose: This example performs a simple step scan of a motor
#           using ordinary motor, scaler, and timer calls.
#
#  Author:  William Lavender
#
#-------------------------------------------------------------------------
#
#  Copyright 2001 Illinois Institute of Technology
#
#  See the file "LICENSE" for information on usage and redistribution
#  of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#
#-------------------------------------------------------------------------
#

import sys

sys.stdout = sys.stderr

#  Define the location of the MX installation directory.

mxdir = "/opt/mx"

#  Define the location of the example MX database.

database_filename = "example.dat"

#  Construct the name of the Mp package directory.

###mp_package_dir = [ "/home/lavender/mxdev/0.56.0/mp/libMp" ]

mp_package_dir = [ mxdir + "lib/mp" ]

#  Add the Mp package directory to the module load path.

sys.path[:0] = mp_package_dir

#  Load the Mp package.

import Mp

#  Read in the MX database used by this example.

record_list = Mp.setup_database( database_filename )

#  Find the devices to be used by the scan.

energy_motor_record = record_list.get_record("energy")

i_zero_record = record_list.get_record("Io")

i_trans_record = record_list.get_record("It")

timer_record = record_list.get_record("timer1")

#  Setup the scan parameters for a copper K edge scan.

start = 8950.0		# in eV
step_size = 1.0		# in eV
num_steps = 101
measurement_time = 1.0	# in seconds

#  Perform the scan.

print "Moving to start position."

#Mp.set_debug_level(2)

for i in range( 0, num_steps ) :

	#  Clear the scalers.

	i_zero_record.clear()

	i_trans_record.clear()

	#  Compute the energy for the next step.

	energy = start + step_size * i

	#  Move to the new energy.

	energy_motor_record.move_absolute(energy)

	#  Wait for the motor to stop moving.

	busy = 1

	while busy :
		busy = energy_motor_record.is_busy()

	#  Start the measurement timer.

	timer_record.start( measurement_time )

	#  Wait until the timer stops counting.

	busy = 1

	while busy :
		busy = timer_record.is_busy()

	#  Read out the scalers.

	i_zero_value = i_zero_record.read()

	i_trans_value = i_trans_record.read()

	#  Print out the values.

	print "%10.3f  %10lu  %10lu" % ( energy, i_zero_value, i_trans_value )

print "Scan complete."

