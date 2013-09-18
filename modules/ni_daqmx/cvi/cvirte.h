/* cvirte.h replacement. */

_declspec(dllimport) int _stdcall InitCVIRTEEx( void *hInstance,
						char *argv[],
						void *reserved );

#define InitCVIRTE	InitCVIRTEEx

_declspec(dllimport) void _stdcall CloseCVIRTE( void );

