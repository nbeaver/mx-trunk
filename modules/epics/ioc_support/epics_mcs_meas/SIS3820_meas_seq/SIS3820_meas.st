program SIS3820_meas

/* FIXME: At the moment, this file is not much better than pseudocode, since
 * I do not yet have enough information about the State Notation Language
 * to get everything syntactically correct.  Think of this as a placeholder.
 * (W. Lavender - 2020-12-04)
 */

/* "option -c' tells the program to start right away.  Do not wait for
 * Channel Access Connections.  This also makes it easier for the sequence
 * program to handle Channel Access disconnection events.
 */

option -c;

/*---*/

string mcs = "vmetest:mcs"

#define MAX_SCALERS		32
#define MAX_MEASUREMENTS	2000

long scaler_data[MAX_SCALERS][MAX_MEASUREMENTS];
assign scaler_data to { "{mcs}:MCA1.VAL", "{mcs}:MCA2.VAL", "{mcs}:MCA3.VAL",
monitor scaler_data;

#define NUM_SCALERS        32

long measurement_range[2] = {-1,-1};
assign measurement_range to "{mcs}:MeasurementRange";
monitor measurement_range;

evflag measurement_range_flag;
sync measurement_range measurement_range_flag;

long measurement_data[100][MAX_SCALERS] = 0;
assign measurement_data to "{mcs}:MeasurementData";

ss measurement_check {
    state init {
        when ( 1 ) {
        } state running
    }

    state running {
        when ( efTestAndClear( measurement_range_flag )) {
            num_measurements = measurement_range[1] - measurement_range[0] + 1;
            measurement_start = measurement_range[0];

            printf(
                "SIS3820_meas: requested %lu measurements starting at %ld.\n",
                        num_measurements, measurement_start );

            for ( meas = 0; meas < num_measurements; meas++ ) {
                for ( scaler = 0; scaler < NUM_SCALERS; scaler++ ) {

                    measurement_data[meas][scaler] = 
                        scaler_data[scaler][meas + measurement_start];
                }
            }

            pvPut( measurement_data );
        } state running
    }
}

