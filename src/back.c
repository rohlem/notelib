#include "back.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "portaudio.h"
#include "notelib/internal.h"

#ifndef NOTELIB_PA_BACK_PROVIDED_BUFFER_SIZE
#define NOTELIB_PA_BACK_PROVIDED_BUFFER_SIZE 0x2000
#endif//#ifndef NOTELIB_PA_BACK_PROVIDED_BUFFER_SIZE

PaStream* pa_back_pa_stream = NULL;

unsigned char pa_back_notelib_buffer[NOTELIB_PA_BACK_PROVIDED_BUFFER_SIZE];
notelib_state_handle pa_back_notelib_handle = NULL;

PaStreamCallback f;

int pa_back_callback
(const void *input, void *output, unsigned long frameCount,
 const PaStreamCallbackTimeInfo* timeInfo,
 PaStreamCallbackFlags statusFlags,
 void *userData){
	notelib_state_handle notelib_state = userData;
	notelib_sample* out = output;
	static unsigned long static_frame_count = 0;
	if(frameCount != static_frame_count){
		printf("\npa_back_callback: frameCount is now %d\n", (int)frameCount);
		static_frame_count = frameCount;
	}
	while(frameCount > 0){
		notelib_sample_uint sample_count;
		if(frameCount > NOTELIB_SAMPLE_UINT_MAX){
			sample_count = NOTELIB_SAMPLE_UINT_MAX;
			frameCount -= NOTELIB_SAMPLE_UINT_MAX;
		}else{
			sample_count = frameCount;
			frameCount = 0;
		}
		notelib_internals_fill_buffer(notelib_state, out, sample_count);
		out += sample_count;
	}
	return paContinue;
}

struct pa_back_init_data pa_back_notelib_initialize(const struct notelib_params* params){
	struct pa_back_init_data ret;

	enum notelib_status ns = notelib_internals_init(pa_back_notelib_buffer, NOTELIB_PA_BACK_PROVIDED_BUFFER_SIZE, params);
	if(ns != notelib_status_ok){
		ret.error.error_type = pa_back_error_notelib;
		ret.error.notelib = ns;
		return ret;
	}
	pa_back_notelib_handle = pa_back_notelib_buffer;

	PaError pe = Pa_Initialize();
	if(pe != paNoError){
		notelib_internals_deinit(pa_back_notelib_handle);
		ret.error.error_type = pa_back_error_backend;
		ret.error.pa = pe;
		return ret;
	}

	PaStreamParameters streamParameters;
	PaDeviceIndex defaultOutputDevice = Pa_GetDefaultOutputDevice();
	if(defaultOutputDevice == paNoDevice){
		Pa_Terminate();
		notelib_internals_deinit(pa_back_notelib_handle);
		ret.error.error_type = pa_back_error_backend;
		ret.error.pa = paInvalidDevice;
		return ret;
	}
	streamParameters.device = defaultOutputDevice;
	PaDeviceInfo const * const defaultOutputDeviceInfo = Pa_GetDeviceInfo(streamParameters.device);
	double defaultSampleRate = defaultOutputDeviceInfo->defaultSampleRate;
	ret.sample_rate = defaultSampleRate;
	streamParameters.channelCount = 1;
	streamParameters.sampleFormat = paInt16;
	streamParameters.suggestedLatency = defaultOutputDeviceInfo->defaultLowOutputLatency;
	streamParameters.hostApiSpecificStreamInfo = NULL;
	PaError pa_error = Pa_OpenStream(&pa_back_pa_stream, NULL, &streamParameters, defaultSampleRate, paFramesPerBufferUnspecified, paNoFlag, &pa_back_callback, pa_back_notelib_handle);
	if(pa_error != paNoError){
		Pa_Terminate();
		notelib_internals_deinit(pa_back_notelib_handle);
		ret.error.error_type = pa_back_error_backend;
		ret.error.pa = pa_error;
		return ret;
	}

	pa_error = Pa_StartStream(pa_back_pa_stream);
	if(pa_error != paNoError){
		Pa_Terminate();
		notelib_internals_deinit(pa_back_notelib_handle);
		ret.error.error_type = pa_back_error_backend;
		ret.error.pa = pa_error;
		return ret;
	}

	ret.error.error_type = pa_back_error_none;
	return ret;
}

struct pa_back_error pa_back_notelib_deinitialize(){
	struct pa_back_error error;
	error.error_type = pa_back_error_none;
	PaError pa_error = Pa_Terminate();
	enum notelib_status ns = notelib_internals_deinit(pa_back_notelib_handle);
	if(ns != notelib_status_ok){
		error.error_type = pa_back_error_notelib;
		error.notelib = ns;
	}
	if(pa_error != paNoError){
		error.error_type = pa_back_error_backend;
		error.pa = pa_error;
	}
	return error;
}
