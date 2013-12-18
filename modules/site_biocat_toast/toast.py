#! /usr/bin/env mpscript

import sys, time

def main( database, argv ):

	if ( len(argv) != 3 ):
		print "Usage: toast 'motor_name' 'start' 'end'"
		sys.exit(0)

	motor_name = argv[0]
	start      = float( argv[1] )
	end        = float( argv[2] )

	motor = database.get_record( motor_name )

	mount = start + ( end - start ) / 2.0

	print "Running toaster.  Press any key to stop."

	try:
		while 1:
			print "Running to %f" % ( start )
	
			motor.move_absolute( start )
	
			motor.wait_for_motor_stop( 0 )
	
			current_position = motor.get_position()
	
			print "Arrived at %f" % ( current_position )
	
			time.sleep(1)
	
			print "Running to %f" % ( end )
	
			motor.move_absolute( end )
	
			motor.wait_for_motor_stop( 0 )
	
			current_position = motor.get_position()
	
			print "Arrived at %f" % ( current_position )
	
			time.sleep(1)
	
	except Mp.Interrupted_Error:

		print "User has interrupted toast."
		print "Exiting and moving toaster to mid_pos=%f" % ( mount )

		# FIXME:
		# The script may die with an exception due to a
		# 'Failed to read a character ...' error on the
		# first attempt to send a command to the Compumotor
		# after a user interrupt.  For now the workaround
		# is to run a "safe" command like get_position()
		# and throw away the error if it occurs.
		#
		# This is probably really an error in the MX 
		# Compumotor drivers.
		try:
			position = motor.get_position()
		except:
			pass

		motor.soft_abort()
	
		motor.wait_for_motor_stop( 0 )
	
		motor.move_absolute( mount )
	
		motor.wait_for_motor_stop( 0 )
	
		print "Move complete."
	
		sys.exit(0)

