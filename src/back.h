#ifndef BACK_H_
#define BACK_H_

#include "notelib/notelib.h"

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

extern notelib_state_handle pa_back_notelib_handle;

struct pa_back_init_data{
	struct notelib_back_error error;
	double sample_rate;
};

struct pa_back_init_data  pa_back_notelib_initialize(const struct notelib_params* params);
struct notelib_back_error pa_back_notelib_deinitialize();


#elif defined(NOTELIB_BACKEND_LIBSOUNDIO) && NOTELIB_BACKEND_LIBSOUNDIO

extern notelib_state_handle sio_back_notelib_handle;

struct sio_back_init_data{
	struct notelib_back_error error;
	double sample_rate;
};

struct sio_back_init_data sio_back_notelib_initialize(const struct notelib_params* params);
struct notelib_back_error sio_back_notelib_deinitialize();


#else
#error NO BACKEND SELECTED
#endif

#ifdef __cplusplus
}//extern "C"
#endif//#ifdef __cplusplus

#endif//#ifndef BACK_H_
