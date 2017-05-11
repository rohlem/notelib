#ifndef BACK_H_
#define BACK_H_

#include "notelib/notelib.h"
#include "portaudio.h"

#ifdef __cplusplus
extern "C" {
#endif//#ifdef __cplusplus

extern notelib_state_handle pa_back_notelib_handle;

struct pa_back_error{
	enum pa_back_error_type{
		pa_back_error_none,
		pa_back_error_backend,
		pa_back_error_notelib
	} error_type;
	union{
		enum notelib_status notelib;
		PaError pa;
	};
};

struct pa_back_init_data{
	struct pa_back_error error;
	double sample_rate;
};

struct pa_back_init_data pa_back_notelib_initialize(const struct notelib_params* params);
struct pa_back_error     pa_back_notelib_deinitialize();


#ifdef __cplusplus
}//extern "C"
#endif//#ifdef __cplusplus

#endif//#ifndef BACK_H_
