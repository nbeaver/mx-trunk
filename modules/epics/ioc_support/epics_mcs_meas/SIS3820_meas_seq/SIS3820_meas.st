/* FIXME: At the moment, this file is not much better than pseudocode, since
 * I do not yet have enough information about the State Notation Language
 * to get everything syntactically correct.  Think of this as a placeholder.
 * (W. Lavender - 2020-12-04)
 */
program SIS3820_meas

string mcs = "vmetest:mcs"

long scaler_data[32][2000];
assign scaler_data to { "{mcs}:MCA1.VAL", "{mcs}:MCA2.VAL", "{mcs}:MCA3.VAL",
                        "{mcs}:MCA4.VAL", "{mcs}:MCA5.VAL", "{mcs}:MCA6.VAL",
                        "{mcs}:MCA7.VAL", "{mcs}:MCA8.VAL", "{mcs}:MCA9.VAL",
                        "{mcs}:MCA10.VAL", "{mcs}:MCA11.VAL", "{mcs}:MCA12.VAL",
                        "{mcs}:MCA13.VAL", "{mcs}:MCA14.VAL", "{mcs}:MCA15.VAL",
                        "{mcs}:MCA16.VAL", "{mcs}:MCA17.VAL", "{mcs}:MCA18.VAL",
                        "{mcs}:MCA19.VAL", "{mcs}:MCA20.VAL", "{mcs}:MCA21.VAL",
                        "{mcs}:MCA22.VAL", "{mcs}:MCA23.VAL", "{mcs}:MCA24.VAL",
                        "{mcs}:MCA25.VAL", "{mcs}:MCA26.VAL", "{mcs}:MCA27.VAL",
                        "{mcs}:MCA28.VAL", "{mcs}:MCA29.VAL", "{mcs}:MCA30.VAL",
                        "{mcs}:MCA31.VAL", "{mcs}:MCA32.VAL" };
monitor scaler_data;

#define NUM_SCALERS        32

long measurement_range[2] = {-1,-1};
assign measurement_range to "{mcs}:MeasurementRange";
monitor measurement_range;
evflag measurement_range_flag;
sync measurement_range measurement_range_flag;

long measurement_data[100][NUM_SCALERS] = 0;
assign measurement_data to "{mcs}:MeasurementData";

ss measurement_check {
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
                                scaler_data[scaler][measurement_start];
                }
            }

            pvPut( measurement_data );
        } state running
    }
}

