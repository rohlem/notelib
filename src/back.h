#ifndef BACK_H_
#define BACK_H_

#include "notelib/notelib.h"
#include "notelib/util/shared_linkage_specifiers.h"

// Even more linkage specifier macros for the backend API.
// If accessed via a backend, the user will never (need to) use this API.

#if defined(NOTELIB_BACKEND_SHARED) && NOTELIB_BACKEND_SHARED // defined as 1 if the backend is to be accessed via a shared library
	#if defined(BUILDING_NOTELIB_SHARED) && BUILDING_NOTELIB_SHARED // defined as 1 if we are building the shared library (instead of using it)
		#define NOTELIB_BACKEND_API NOTELIB_UTIL_SHARED_EXPORT
	#else
		#define NOTELIB_BACKEND_API NOTELIB_UTIL_SHARED_IMPORT
	#endif // BUILDING_NOTELIB_SHARED
	#define NOTELIB_BACKEND_LOCAL NOTELIB_UTIL_SHARED_LOCAL
#else // NOTELIB_BACKEND_SHARED is not defined as 1: this means initialization and shutdown of the backend will not be done via the shared library interface
	#define NOTELIB_BACKEND_API
	#define NOTELIB_BACKEND_LOCAL
#endif // NOTELIB_BACKEND_SHARED

#if defined(NOTELIB_BACKEND_PORTAUDIO) && NOTELIB_BACKEND_PORTAUDIO
#include "portaudio.h"
#elif defined(NOTELIB_BACKEND_LIBSOUNDIO) && NOTELIB_BACKEND_LIBSOUNDIO
#include "soundio/soundio.h"
#endif


#ifdef __cplusplus
extern "C" {
#endif//#ifdef __cplusplus

struct notelib_back_error{
	enum notelib_back_error_type{
		notelib_back_error_none,
		notelib_back_error_backend,
		notelib_back_error_notelib
	} error_type;
	union{
		enum notelib_status notelib;
		#if defined(NOTELIB_BACKEND_PORTAUDIO) && NOTELIB_BACKEND_PORTAUDIO
			PaError pa;
		#endif
		#if defined(NOTELIB_BACKEND_LIBSOUNDIO) && NOTELIB_BACKEND_LIBSOUNDIO
			enum SoundIoError sio;
		#endif
	};
};

#if defined(NOTELIB_BACKEND_PORTAUDIO) && NOTELIB_BACKEND_PORTAUDIO

NOTELIB_BACKEND_API extern notelib_state_handle pa_back_notelib_handle;

struct pa_back_init_data{
	struct notelib_back_error error;
	double sample_rate;
};

NOTELIB_BACKEND_API struct pa_back_init_data  pa_back_notelib_initialize(const struct notelib_params* params);
NOTELIB_BACKEND_API struct notelib_back_error pa_back_notelib_deinitialize();


#elif defined(NOTELIB_BACKEND_LIBSOUNDIO) && NOTELIB_BACKEND_LIBSOUNDIO

NOTELIB_BACKEND_API extern notelib_state_handle sio_back_notelib_handle;

struct sio_back_init_data{
	struct notelib_back_error error;
	double sample_rate;
};

NOTELIB_BACKEND_API struct sio_back_init_data sio_back_notelib_initialize(const struct notelib_params* params);
NOTELIB_BACKEND_API struct notelib_back_error sio_back_notelib_deinitialize();


#else
#error NO BACKEND SELECTED
#endif

#ifdef __cplusplus
}//extern "C"
#endif//#ifdef __cplusplus

#endif//#ifndef BACK_H_
