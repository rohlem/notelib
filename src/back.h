#ifndef BACK_H_
#define BACK_H_

#include "notelib/notelib.h"
#include "notelib/util/shared_linkage_specifiers.h"


// Even more linkage specifier macros for the backend API.
// If only accessed via a backend, the user will never (need to) use the "internal" state management API.

#if defined(NOTELIB_BACKEND_SHARED) && NOTELIB_BACKEND_SHARED // defined as 1 if the backend API is to be accessed via a shared library
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

// These are for the "backend specific API", which gives full access to the underlying API but precludes backend-agnostic error "handling" ABI.

#if defined(NOTELIB_BACKEND_SPECIFIC_SHARED) && NOTELIB_BACKEND_SPECIFIC_SHARED // defined as 1 if the backend-specific API is to be accessed via a shared library
	#if defined(BUILDING_NOTELIB_SHARED) && BUILDING_NOTELIB_SHARED // defined as 1 if we are building the shared library (instead of using it)
		#define NOTELIB_BACKEND_SPECIFIC_API NOTELIB_UTIL_SHARED_EXPORT
	#else
		#define NOTELIB_BACKEND_SPECIFIC_API NOTELIB_UTIL_SHARED_IMPORT
	#endif // BUILDING_NOTELIB_SHARED
	#define NOTELIB_BACKEND_SPECIFIC_LOCAL NOTELIB_UTIL_SHARED_LOCAL
#else // NOTELIB_BACKEND_SPECIFIC_SHARED is not defined as 1: this means initialization and shutdown of specific backends will not be done via the shared library interface
	#define NOTELIB_BACKEND_SPECIFIC_API
	#define NOTELIB_BACKEND_SPECIFIC_LOCAL
#endif // NOTELIB_BACKEND_SPECIFIC_SHARED

#if defined(NOTELIB_BACKEND_PORTAUDIO) && NOTELIB_BACKEND_PORTAUDIO
#include "portaudio.h"
#endif
#if defined(NOTELIB_BACKEND_LIBSOUNDIO) && NOTELIB_BACKEND_LIBSOUNDIO
#include "soundio/soundio.h"
#endif


#ifdef __cplusplus
extern "C" {
#endif//#ifdef __cplusplus


enum notelib_backend_error_type{
	notelib_backend_error_none,
	notelib_backend_error_notelib,
	notelib_backend_error_backend,
};
struct notelib_backend_generic_error{
	enum notelib_backend_error_type type;
	enum notelib_status notelib;
};
struct notelib_backend_generic_audio_info{
	double sample_rate;
};
struct notelib_backend_generic_init_data{
	notelib_state_handle notelib_state; //initialized to NULL in case of error
	struct notelib_backend_generic_audio_info audio_info;
	struct notelib_backend_generic_error error_info; //error_type holds notelib_backend_error_none if notelib_state is non-NULL
};

// pointers to NULL-terminated array of pointers to NULL-terminated char arrays (strings)
struct notelib_backend_generic_info{
	const char* name;
	const char* version;
	const char* error_description;
};
NOTELIB_BACKEND_API extern const struct notelib_backend_generic_info* const notelib_backend_info; // NULL-terminated array of unspecified size (-> pointer to contents of unspecified size); note that size >= 1, since the NULL terminating element is guaranteed to be present

NOTELIB_BACKEND_API struct notelib_backend_generic_init_data notelib_backend_initialize(const struct notelib_params* params); //result.notelib_state has to be deinitialized by notelib_backend_deinitialize (no other function!!)
NOTELIB_BACKEND_API struct notelib_backend_generic_error notelib_backend_deinitialize(notelib_state_handle notelib_state); //notelib_state has to have come from notelib_backend_initialize (no other function!!)

NOTELIB_BACKEND_API struct notelib_backend_generic_init_data notelib_backend_initialize_in(void* buffer, size_t buffer_size, const struct notelib_params* params); //uses preallocated memory - to be used with notelib_backend_deinitialize
NOTELIB_BACKEND_API struct notelib_backend_generic_error notelib_backend_deinitialize_no_free(notelib_state_handle notleib_state); //does not free memory - to be used with notelib_backend_initialize_in


#if defined(NOTELIB_BACKEND_LIBSOUNDIO) && NOTELIB_BACKEND_LIBSOUNDIO


	struct notelib_backend_libsoundio_error{
		enum notelib_backend_error_type type;
		union{
			enum notelib_status notelib;
			enum SoundIoError sio;
		};
	};
	struct notelib_backend_libsoundio_data{
		//all members are initialized to NULL in case of error
		notelib_state_handle notelib_state;
		struct notelib_backend_libsoundio_specific_data{
			struct SoundIo* instance;
			struct SoundIoDevice* device;
			struct SoundIoOutStream* outstream;
		} sio_state;
	};
	struct notelib_backend_libsoundio_init_data{
		struct notelib_backend_libsoundio_data initialized_state;
		struct notelib_backend_libsoundio_error error_info; //error_type holds notelib_backend_error_none if notelib_state is non-NULL
	};

	NOTELIB_BACKEND_SPECIFIC_API struct notelib_backend_libsoundio_init_data notelib_backend_libsoundio_initialize(const struct notelib_params* params); //allocates memory - to be used with _deinitialize
	NOTELIB_BACKEND_SPECIFIC_API struct notelib_backend_libsoundio_error notelib_backend_libsoundio_deinitialize(struct notelib_backend_libsoundio_data* backend_state); //frees allocated memory - to be used with _initialize

	NOTELIB_BACKEND_SPECIFIC_API struct notelib_backend_libsoundio_init_data notelib_backend_libsoundio_initialize_in(void* buffer, size_t buffer_size, const struct notelib_params* params); //uses preallocated memory - to be used with _deinitialize_no_free
	NOTELIB_BACKEND_SPECIFIC_API struct notelib_backend_libsoundio_error notelib_backend_libsoundio_deinitialize_no_free(struct notelib_backend_libsoundio_data* backend_state); //does not free memory - to be used with _initialize_in


#endif
#if defined(NOTELIB_BACKEND_PORTAUDIO) && NOTELIB_BACKEND_PORTAUDIO


	struct notelib_backend_portaudio_error{
		enum notelib_backend_error_type type;
		union{
			enum notelib_status notelib;
			PaError pa;
		};
	};
	struct notelib_backend_portaudio_data{
		//all members are initialized to NULL in case of error
		notelib_state_handle notelib_state;
		struct notelib_backend_portaudio_specific_data{
			PaStream* stream_handle;
		} pa_state;
	};
	struct notelib_backend_portaudio_init_data{
		struct notelib_backend_portaudio_data initialized_state;
		struct notelib_backend_portaudio_error error_info; //error_type holds notelib_backend_error_none if notelib_state is non-NULL
	};

	NOTELIB_BACKEND_SPECIFIC_API struct notelib_backend_portaudio_init_data notelib_backend_portaudio_initialize(const struct notelib_params* params); //allocates memory - to be used with _deinitialize
	NOTELIB_BACKEND_SPECIFIC_API struct notelib_backend_portaudio_error notelib_backend_portaudio_deinitialize(struct notelib_backend_portaudio_data* backend_state); //frees allocated memory - to be used with _initialize

	NOTELIB_BACKEND_SPECIFIC_API struct notelib_backend_portaudio_init_data notelib_backend_portaudio_initialize_in(void* buffer, size_t buffer_size, const struct notelib_params* params); //uses preallocated memory - to be used with _deinitialize_no_free
	NOTELIB_BACKEND_SPECIFIC_API struct notelib_backend_portaudio_error notelib_backend_portaudio_deinitialize_no_free(struct notelib_backend_portaudio_data* backend_state); //does not free memory - to be used with _initialize_in


#endif

#ifdef __cplusplus
}//extern "C"
#endif//#ifdef __cplusplus

#endif//#ifndef BACK_H_
