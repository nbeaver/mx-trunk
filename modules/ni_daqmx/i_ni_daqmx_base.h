/*
 * Name:    i_ni_daqmx_base.h
 *
 * Purpose: Header for compiling the MX 'ni_daqmx' module using the
 *          National Instruments DAQmx Base package.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2012 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_NI_DAQMX_BASE_H__
#define __I_NI_DAQMX_BASE_H__

#define DAQmxClearTask( a )		DAQmxBaseClearTask( a )

#define DAQmxCreateAIVoltageChan( a, b, c, d, e, f, g, h, i, j )  \
	DAQmxBaseCreateAIVoltageChan((a),(b),(c),(d),(e),(f),(g),(h),(i),(j))

#define DAQmxCreateAIVoltageChan( a, b, c, d, e, f, g, h )  \
		DAQmxBaseCreateAIVoltageChan((a),(b),(c),(d),(e),(f),(g),(h))

#define DAQmxCreateAOVoltageChan( a, b, c, d, e, f, g )  \
		DAQmxBaseCreateAOVoltageChan((a),(b),(c),(d),(e),(f),(g))

#define DAQmxCreateDIChan( a, b, c, d )	DAQmxBaseCreateDIChan((a),(b),(c),(d))

#define DAQmxCreateDOChan( a, b, c, d )	DAQmxBaseCreateDOChan((a),(b),(c),(d))

#define DAQmxCreateTask( a, b )		DAQmxBaseCreateTask((a),(b))

#define DAQmxGetExtendedErrorInfo( a, b ) \
		DAQmxBaseGetExtendedErrorInfo((a),(b))

#define DAQmxReadAnalogF64( a, b, c, d, e, f, g, h ) \
		DAQmxBaseReadAnalogF64((a),(b),(c),(d),(e),(f),(g),(h))

#define DAQmxReadDigitalU32( a, b, c, d, e, f, g, h )  \
		DAQmxBaseReadDigitalU32((a),(b),(c),(d),(e),(f),(g),(h))

#define DAQmxStartTask( a )		DAQmxBaseStartTask( a )

#define DAQmxStopTask( a )		DAQmxBaseStopTask( a )

#define DAQmxWriteAnalogF64( a, b, c, d, e, f, g, h )  \
		DAQmxBaseWriteAnalogF64((a),(b),(c),(d),(e),(f),(g),(h))

#define DAQmxWriteDigitalU32( a, b, c, d, e, f, g, h )  \
		DAQmxBaseWriteDigitalU32((a),(b),(c),(d),(e),(f),(g),(h))

#endif /* __I_NI_DAQMX_BASE_H__ */
