#! /usr/bin/env mpscript

import sys

motor = None
mount = 0.0

def user_interrupt():
	print "User has interrupted toast."
	print "Exiting and moving toaster to mid_pos=%f" % ( mount )

	motor.move_absolute( mount )

	motor.wait_for_motor_stop( 0 )

	print "Move complete."

	sys.exit(0)

def main( database, argv ):
	global motor
	global mount

	start      = -0.5
	end        =  0.5
	motor_name = "m1"

	motor = database.get_record( motor_name )

	mount = start + ( end - start ) / 2.0

	Mp.set_user_interrupt_function( user_interrupt )

	print "Running toaster.  Press any key to stop."

	while 1:
		print "Running to %f" % ( start )

		motor.move_absolute( start )

		motor.wait_for_motor_stop( 0 )

		current_position = motor.get_position()

		print "Arrived at %f" % ( current_position )

		print "Running to %f" % ( end )

		motor.move_absolute( end )

		motor.wait_for_motor_stop( 0 )

		current_position = motor.get_position()

		print "Arrived at %f" % ( current_position )

