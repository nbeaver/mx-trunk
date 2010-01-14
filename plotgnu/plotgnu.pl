#!/usr/bin/perl

$os = $^O;

if ( $os eq "MSWin32" ) {
	$tempfile = $ENV{'TMP'} . "/plotgnu.tmp$$";
} elsif ( $os eq "cygwin" ) {
	$tempfile = "/tmp/plotgnu.tmp$$";
} else {
	$tempfile = "/tmp/plotgnu.tmp$$";
}

# Save and then redirect STDERR so that output to STDERR by gnuplot 
# is discarded.

open(SAVEERR, ">&STDERR");

open(STDERR, ">/dev/null");

# Open the pipe to gnuplot.  On Win32, we use a proxy called mxgnuplt.

if ( $os eq "MSWin32" ) {
	open(GNUPLOT, "| mxgnuplt") || die "Cannot open pipe to gnuplot";
} elsif ( $os eq "cygwin" ) {
	open(GNUPLOT, "| mxgnuplt") || die "Cannot open pipe to gnuplot";
} else {
	open(GNUPLOT, "| gnuplot") || die "Cannot open pipe to gnuplot";
}

# Restore STDERR.

open(STDERR, ">&SAVEERR");

# Do not buffer the gnuplot pipe.

$old_select = select(GNUPLOT);
$| = 1;    
select($old_select);

print GNUPLOT "set data style linespoints\n";

# Handle commands from the parent program.

while ($input_line = <STDIN>) {

	chop $input_line;  # Get rid of the newline at the end of the string.

	###print STDERR "input_line = '$input_line'\n";

	# Handle special commands first.

	if ( $input_line eq "exit" ) {
		close(OUTPUT);
		print GNUPLOT "exit\n";
		close(GNUPLOT);
		unlink $tempfile;
		exit;
	}
	if ( substr( $input_line, 0, 4 ) eq "set " ) {
		print GNUPLOT "$input_line\n";
		next;	# Skip the rest of the block and go back to while(1).
	}

	if ( substr( $input_line, 0, 10 ) eq "start_plot" ) {

		@parameters = split( /;/, $input_line );

		shift @parameters;	# Get rid of 'start_plot' token.

		$num_independent_variables = $parameters[0];
		shift @parameters;

		$innermost_loop_motor_index = $parameters[0];
		shift @parameters;

		if ( $innermost_loop_motor_index < 0 ) {
			$innermost_loop_motor_index = 0;
		}

		# The innermost loop motor must be inserted at the beginning
		# of the plot parameters array.  That is because it serves
		# as the x axis of the plot.

		@parameters = ("\$m[$innermost_loop_motor_index]", @parameters);

		# Close the old output file, if it exists.

		close(OUTPUT);

		# Open the output file, which is used as the plot file
		# for gnuplot.

		$status = open(OUTPUT, ">$tempfile");

		if ( ! defined( $status ) ) {
			print GNUPLOT "exit\n";
			die "Cannot open $tempfile";
		}

		# Do not buffer the output file.

		$old_select = select(OUTPUT);
		$| = 1;
		select($old_select);

		next;	# Skip the rest of the block and go back to while(1).
	}

	if ( substr( $input_line, 0, 4 ) eq "data" ) {

		if ( ! defined(@parameters) ) {
		    print GNUPLOT "exit\n";

		    die "Attempted to send plot data before starting a plot";
		}

		@f = split( " ", $input_line );

		shift @f;	# To get rid of the part that says 'data'.

		# Put the independent variables at the beginning of the array
		# into their own separate array.

		@m = ();	# A null array.

		for ( $i = 0; $i < $num_independent_variables; $i++ ) {
			@m = (@m, $f[0]);

			shift @f;
		}

		for ( $i = 0; $i <= $#parameters; $i++ ) {
			$result = eval "$parameters[$i]";

			if ( ! defined($result) ) {
				print STDERR
	"Error evaluating expression '$parameters[$i]'.  Result set to 0.\n";

				$result = 0;
			}

			# 
			# Verify that the result is an integer or floating
			# point number.
			#
			# The following rather hairy expression was copied
			# verbatim from the Perl FAQ found at www.perl.com.
			#
			#=================================================
			#
			# Perl version 4 generates an error message if
			# the following block of code is not commented out.
			# If you are running Perl version 5, you can 
			# uncomment this section and thereby enable 
			# error checking to verify that $result really is
			# a number.
			#
#
#			unless ( $result =~
#			  /^([+-]?)(?=\d|\.\d)\d*(\.\d*)?([Ee]([+-]?\d+))?$/ ){
#
#				print GNUPLOT "exit\n";
#
#				die "Non-numeric expression '$parameters[$i]'";
#			}

			print OUTPUT $result . " ";
		}
		print OUTPUT "\n";

		next;	# Skip the rest of the block and go back to while(1).
	}

	if ( substr( $input_line, 0, 4 ) eq "plot" ) {

		if ( ! defined(@parameters) ) {
		    print GNUPLOT "exit\n";

    die "Attempted to tell gnuplot to plot before the plot file was created.";
		}

		# Tell gnuplot to replot the data.

		print GNUPLOT "plot ";

		for ( $i = 1; $i <= $#parameters; $i++ ) {
			$j = $i + 1;

			print GNUPLOT
			    "'$tempfile' using 1:$j title '$parameters[$i]'";

			if ( $i != $#parameters ) {
				print GNUPLOT ", ";
			}
		}
		print GNUPLOT "\n";

		next;	# Skip the rest of the block and go back to while(1).
	}

	# If we get here, we have gotten an unrecognized command.

	print GNUPLOT "exit\n";

	die "Unrecognized command '$input_line' passed";
}

# If we get here, our parent process has gone away unexpectedly, so we must
# shut down gnuplot before exiting.

print GNUPLOT "exit\n";

