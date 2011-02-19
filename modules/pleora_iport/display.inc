/*------------------------------------------------------------------------*/

static void
mxi_pleora_iport_display_extension_info( CyDeviceExtension *extension,
			unsigned long parameter_id )
{
	CyString cy_string;
	unsigned long parameter_type;
	__int64 int64_min, int64_max, int64_value;
	double double_min, double_max, double_value;

	parameter_type = extension->GetParameterType( parameter_id );

	CyString parameter_name =
			extension->GetParameterName( parameter_id );

	fprintf( stderr, "Param (%lu) '%s', ",
			parameter_id, parameter_name.c_str_ascii() );

	switch( parameter_type ) {
	case CY_PARAMETER_STRING:
		extension->GetParameter( parameter_type, cy_string );

		fprintf( stderr, "string  '%s'\n",
			cy_string.c_str_ascii() );
		break;

	case CY_PARAMETER_INT:
		extension->GetParameter( parameter_id, int64_value );

		extension->GetParameterRange( parameter_id,
					int64_min, int64_max );

		fprintf( stderr, " int64  %lI64d (%lI64d to %lI64d)\n",
				int64_value, int64_min, int64_max );
		break;

	case CY_PARAMETER_DOUBLE:
		extension->GetParameter( parameter_id, double_value );

		extension->GetParameterRange( parameter_id,
					double_min, double_max );

		fprintf( stderr, "double  %g (%g to %g)\n",
				double_value, double_min, double_max );
		break;

	case CY_PARAMETER_BOOL:
		extension->GetParameter( parameter_id, int64_value );

		extension->GetParameterRange( parameter_id,
					int64_min, int64_max );

		fprintf( stderr, " bool  %lI64d (%lI64d to %lI64d)\n",
				int64_value, int64_min, int64_max );
		break;

	case CY_PARAMETER_ENUM:
		extension->GetParameter( parameter_id, int64_value );

		extension->GetParameterRange( parameter_id,
					int64_min, int64_max );

		fprintf( stderr, " enum  %lI64d (%lI64d to %lI64d)\n",
				int64_value, int64_min, int64_max );
		break;

	default:
		fprintf( stderr, "unrecognized type %lu\n",
					parameter_type );
		break;
	}

	return;
}

static void
mxi_pleora_iport_display_extension_array( CyDeviceExtension *extension,
					unsigned long num_parameters,
					unsigned long *parameter_array )
{
	unsigned long i, parameter_id;

	const char *extension_name = extension->GetName().c_str_ascii();

	fprintf(stderr, "*************************************************\n");
	fprintf(stderr, "Extension '%s'\n", extension_name);

	for ( i = 0; i < num_parameters; i++ ) {
		parameter_id = parameter_array[i];

		mxi_pleora_iport_display_extension_info( extension,
							parameter_id );
	}
	fprintf(stderr, "*************************************************\n");
}

/*------------------------------------------------------------------------*/

static void
mxi_pleora_iport_display_grabber_info( CyGrabber *grabber,
			unsigned long parameter_id )
{
	CyString cy_string;
	unsigned long parameter_type;
	__int64 int64_min, int64_max, int64_value;
	double double_min, double_max, double_value;

	parameter_type = grabber->GetParameterType( parameter_id );

	CyString parameter_name =
			grabber->GetParameterName( parameter_id );

	fprintf( stderr, "Param (%lu) '%s', ",
			parameter_id, parameter_name.c_str_ascii() );

	switch( parameter_type ) {
	case CY_PARAMETER_STRING:
		grabber->GetParameter( parameter_type, cy_string );

		fprintf( stderr, "string  '%s'\n",
			cy_string.c_str_ascii() );
		break;

	case CY_PARAMETER_INT:
		grabber->GetParameter( parameter_id, int64_value );

		grabber->GetParameterRange( parameter_id,
					int64_min, int64_max );

		fprintf( stderr, " int64  %lI64d (%lI64d to %lI64d)\n",
				int64_value, int64_min, int64_max );
		break;

	case CY_PARAMETER_DOUBLE:
		grabber->GetParameter( parameter_id, double_value );

		grabber->GetParameterRange( parameter_id,
					double_min, double_max );

		fprintf( stderr, "double  %g (%g to %g)\n",
				double_value, double_min, double_max );
		break;

	case CY_PARAMETER_BOOL:
		grabber->GetParameter( parameter_id, int64_value );

		grabber->GetParameterRange( parameter_id,
					int64_min, int64_max );

		fprintf( stderr, " bool  %lI64d (%lI64d to %lI64d)\n",
				int64_value, int64_min, int64_max );
		break;

	case CY_PARAMETER_ENUM:
		grabber->GetParameter( parameter_id, int64_value );

		grabber->GetParameterRange( parameter_id,
					int64_min, int64_max );

		fprintf( stderr, " enum  %lI64d (%lI64d to %lI64d)\n",
				int64_value, int64_min, int64_max );
		break;

	default:
		fprintf( stderr, "unrecognized type %lu\n",
					parameter_type );
		break;
	}

	return;
}

static void
mxi_pleora_iport_display_grabber_array( CyGrabber *grabber,
					unsigned long num_parameters,
					unsigned long *parameter_array )
{
	unsigned long i, parameter_id;

	const char *grabber_name = grabber->GetName().c_str_ascii();

	fprintf(stderr, "*************************************************\n");
	fprintf(stderr, "Grabber '%s'\n", grabber_name);

	for ( i = 0; i < num_parameters; i++ ) {
		parameter_id = parameter_array[i];

		mxi_pleora_iport_display_grabber_info( grabber,
							parameter_id );
	}
	fprintf(stderr, "*************************************************\n");
}

/*------------------------------------------------------------------------*/

void mxi_pleora_iport_display_all_parameters( CyGrabber *grabber )
{
	CyDevice &device = grabber->GetDevice();

	unsigned long gr_parameter_array[] = {
		CY_GRABBER_PARAM_IMAGE_SIZE,
		CY_GRABBER_PARAM_NORMALIZED,
		CY_GRABBER_PARAM_OFFSET_X,
		CY_GRABBER_PARAM_OFFSET_Y,
		CY_GRABBER_PARAM_PACKED,
		CY_GRABBER_PARAM_PIXEL_DEPTH,
		CY_GRABBER_PARAM_SIZE_X,
		CY_GRABBER_PARAM_SIZE_Y,
		CY_GRABBER_PARAM_TAP_QUANTITY };

	unsigned long gr_num_parameters
		= sizeof(gr_parameter_array)
		/ sizeof(gr_parameter_array[0]);

	mxi_pleora_iport_display_grabber_array( grabber,
				gr_num_parameters, gr_parameter_array );

	CyDeviceExtension *pg_extension =
		&device.GetExtension( CY_DEVICE_EXT_PULSE_GENERATOR );

	unsigned long pg_parameter_array[] = {
		CY_PULSE_GEN_PARAM_DELAY,
		CY_PULSE_GEN_PARAM_FREQUENCY,
		CY_PULSE_GEN_PARAM_GRANULARITY,
		CY_PULSE_GEN_PARAM_PERIOD,
		CY_PULSE_GEN_PARAM_PERIODIC,
		CY_PULSE_GEN_PARAM_TRIGGER_MODE,
		CY_PULSE_GEN_PARAM_WIDTH };

	unsigned long pg_num_parameters
		= sizeof(pg_parameter_array)
		/ sizeof(pg_parameter_array[0]);

	mxi_pleora_iport_display_extension_array( pg_extension,
				pg_num_parameters, pg_parameter_array );

	CyDeviceExtension *lut_extension =
		&device.GetExtension( CY_DEVICE_EXT_GPIO_LUT );

	unsigned long lut_parameter_array[] = {
		CY_GPIO_LUT_PARAM_GPIO_LUT_PROGRAM,
		CY_GPIO_LUT_PARAM_INPUT_CONFIG0,
		CY_GPIO_LUT_PARAM_INPUT_CONFIG2,
		CY_GPIO_LUT_PARAM_INPUT_CONFIG7 };

	unsigned long lut_num_parameters
		= sizeof(lut_parameter_array)
		/ sizeof(lut_parameter_array[0]);

	mxi_pleora_iport_display_extension_array( lut_extension,
				lut_num_parameters, lut_parameter_array );
}

/*------------------------------------------------------------------------*/
