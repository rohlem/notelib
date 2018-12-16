#include "back.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "notelib/internal.h"

#if defined(NOTELIB_BACKEND_PORTAUDIO) && NOTELIB_BACKEND_PORTAUDIO

#include "portaudio.h"

#ifndef NOTELIB_PA_BACK_PROVIDED_BUFFER_SIZE
#define NOTELIB_PA_BACK_PROVIDED_BUFFER_SIZE 0x2000
#endif//#ifndef NOTELIB_PA_BACK_PROVIDED_BUFFER_SIZE

PaStream* pa_back_pa_stream = NULL;

unsigned char pa_back_notelib_buffer[NOTELIB_PA_BACK_PROVIDED_BUFFER_SIZE];
notelib_state_handle pa_back_notelib_handle = NULL;

int pa_back_callback
(const void *input, void *output, unsigned long frameCount,
 const PaStreamCallbackTimeInfo* timeInfo,
 PaStreamCallbackFlags statusFlags,
 void *userData){
	(void)input;
	(void)timeInfo;
	(void)statusFlags;
	notelib_state_handle notelib_state = userData;
	notelib_sample* out = output;
/*	static unsigned long static_frame_count = 0;
	if(frameCount != static_frame_count){
		printf("\npa_back_callback: frameCount is now %d\n", (int)frameCount);
		static_frame_count = frameCount;
	}*/
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

struct notelib_back_init_data pa_back_notelib_initialize(const struct notelib_params* params){
	struct notelib_back_init_data ret;

	enum notelib_status ns = notelib_internals_init(pa_back_notelib_buffer, NOTELIB_PA_BACK_PROVIDED_BUFFER_SIZE, params);
	if(ns != notelib_status_ok){
		ret.error.error_type = notelib_back_error_notelib;
		ret.error.notelib = ns;
		return ret;
	}
	pa_back_notelib_handle = pa_back_notelib_buffer;

	PaError pe = Pa_Initialize();
	if(pe != paNoError){
		notelib_internals_deinit(pa_back_notelib_handle);
		ret.error.error_type = notelib_back_error_backend;
		ret.error.pa = pe;
		return ret;
	}

	PaStreamParameters streamParameters;
	PaDeviceIndex defaultOutputDevice = Pa_GetDefaultOutputDevice();
	if(defaultOutputDevice == paNoDevice){
		Pa_Terminate();
		notelib_internals_deinit(pa_back_notelib_handle);
		ret.error.error_type = notelib_back_error_backend;
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
		ret.error.error_type = notelib_back_error_backend;
		ret.error.pa = pa_error;
		return ret;
	}

	pa_error = Pa_StartStream(pa_back_pa_stream);
	if(pa_error != paNoError){
		Pa_Terminate();
		notelib_internals_deinit(pa_back_notelib_handle);
		ret.error.error_type = notelib_back_error_backend;
		ret.error.pa = pa_error;
		return ret;
	}

	ret.error.error_type = notelib_back_error_none;
	return ret;
}

struct notelib_back_error pa_back_notelib_deinitialize(){
	struct notelib_back_error error;
	error.error_type = notelib_back_error_none;
	PaError pa_error = Pa_Terminate();
	enum notelib_status ns = notelib_internals_deinit(pa_back_notelib_handle);
	if(ns != notelib_status_ok){
		error.error_type = notelib_back_error_notelib;
		error.notelib = ns;
	}
	if(pa_error != paNoError){
		error.error_type = notelib_back_error_backend;
		error.pa = pa_error;
	}
	return error;
}

#elif defined(NOTELIB_BACKEND_LIBSOUNDIO) && NOTELIB_BACKEND_LIBSOUNDIO


#ifndef NOTELIB_SIO_BACK_PROVIDED_BUFFER_SIZE
#define NOTELIB_SIO_BACK_PROVIDED_BUFFER_SIZE 0x2000
#endif//#ifndef NOTELIB_SIO_BACK_PROVIDED_BUFFER_SIZE

struct SoundIo* sio_back_sio_instance = NULL;
struct SoundIoDevice* sio_back_sio_device = NULL;
struct SoundIoOutStream* sio_back_sio_outstream = NULL;

unsigned char sio_back_notelib_buffer[NOTELIB_SIO_BACK_PROVIDED_BUFFER_SIZE];
notelib_state_handle sio_back_notelib_handle = NULL;


static int imin(int a, int b) {return a <= b ? a : b;}
static void write_sample_float32ne(char *ptr, notelib_sample sample) {
	*((float*)ptr) = sample;
}
static notelib_sample sio_back_intermediate_audio_buffer[1<<12];
static void write_callback(struct SoundIoOutStream *outstream, int frame_count_min, int frame_count_max) {
	struct SoundIoChannelArea *areas;
	int err;

	int frames_left = frame_count_min;(void)frame_count_max;//min(frame_count_max, max(/*10 ms*/float_sample_rate*.01, frame_count_min));

	while(frames_left > 0){
		int frame_count = frames_left;
		if((err = soundio_outstream_begin_write(outstream, &areas, &frame_count)))
			goto unrecoverable_error;

		if(!frame_count)
			break;

		const struct SoundIoChannelLayout *layout = &outstream->layout;
		if(layout->channel_count <= 0) break;

		int frames_to_generate = frame_count;
		while(frames_to_generate > 0){
			int const generated_frames = imin(frames_to_generate, sizeof(sio_back_intermediate_audio_buffer) / (sizeof(*sio_back_intermediate_audio_buffer)));
			notelib_internals_fill_buffer(sio_back_notelib_handle, sio_back_intermediate_audio_buffer, generated_frames);
			for(notelib_sample_uint i = 0; (int)i < frames_to_generate; ++i){
				notelib_sample sample = sio_back_intermediate_audio_buffer[i];
				for(int channel = 0; channel < layout->channel_count; ++channel){
					write_sample_float32ne(areas[channel].ptr, sample);
					areas[channel].ptr += areas[channel].step;
				}
			}
			frames_to_generate -= generated_frames;
		}

		if((err = soundio_outstream_end_write(outstream))) {
			if(err == SoundIoErrorUnderflow)
				return;
			goto unrecoverable_error;
		}

		frames_left -= frame_count;
	}
	return;
	unrecoverable_error:;
		static bool unrecoverable_error_occurred = false;
		if(!unrecoverable_error_occurred){
			fprintf(stderr, "libsoundio notelib backend: unrecoverable stream error: %s\n", soundio_strerror(err));
			unrecoverable_error_occurred = true;
		}
}

static void underflow_callback(struct SoundIoOutStream *outstream){
	(void)outstream;
	static int count = 0;
	fprintf(stderr, "underflow %d\n", count++);
}

struct sio_back_init_data sio_back_notelib_initialize(const struct notelib_params* params){
	struct sio_back_init_data ret;

	enum notelib_status ns = notelib_internals_init(sio_back_notelib_buffer, NOTELIB_SIO_BACK_PROVIDED_BUFFER_SIZE, params);
	if(ns != notelib_status_ok){
		ret.error.error_type = notelib_back_error_notelib;
		ret.error.notelib = ns;
		return ret;
	}
	sio_back_notelib_handle = sio_back_notelib_buffer;
	sio_back_sio_instance = soundio_create();
	if(!sio_back_sio_instance){
		ret.error.sio = SoundIoErrorNoMem;
		ret.error.error_type = notelib_back_error_backend;
		notelib_internals_deinit(sio_back_notelib_handle);
		return ret;
	}

	enum SoundIoError err = soundio_connect(sio_back_sio_instance);
	if(err){
		ret.error.sio = err;
		soundio_destroy(sio_back_sio_instance);
		sio_back_sio_instance = NULL;
		ret.error.error_type = notelib_back_error_backend;
		notelib_internals_deinit(sio_back_notelib_handle);
		return ret;
	}

	soundio_flush_events(sio_back_sio_instance);

	int selected_device_index = soundio_default_output_device_index(sio_back_sio_instance);
	if(selected_device_index < 0){
		ret.error.sio = SoundIoErrorNoSuchDevice;
		soundio_destroy(sio_back_sio_instance);
		sio_back_sio_instance = NULL;
		ret.error.error_type = notelib_back_error_backend;
		notelib_internals_deinit(sio_back_notelib_handle);
		return ret;
	}

	sio_back_sio_device = soundio_get_output_device(sio_back_sio_instance, selected_device_index);
	if(!sio_back_sio_device) {
		ret.error.sio = SoundIoErrorOpeningDevice;
		soundio_destroy(sio_back_sio_instance);
		sio_back_sio_instance = NULL;
		ret.error.error_type = notelib_back_error_backend;
		notelib_internals_deinit(sio_back_notelib_handle);
		return ret;
	}

	if((ret.error.sio = sio_back_sio_device->probe_error)) {
		soundio_device_unref(sio_back_sio_device);
		sio_back_sio_device = NULL;
		soundio_destroy(sio_back_sio_instance);
		sio_back_sio_instance = NULL;
		ret.error.error_type = notelib_back_error_backend;
		notelib_internals_deinit(sio_back_notelib_handle);
		return ret;
	}

	sio_back_sio_outstream = soundio_outstream_create(sio_back_sio_device);
	if(!sio_back_sio_outstream) {
		ret.error.sio = SoundIoErrorNoMem;
		soundio_device_unref(sio_back_sio_device);
		sio_back_sio_device = NULL;
		soundio_destroy(sio_back_sio_instance);
		sio_back_sio_instance = NULL;
		ret.error.error_type = notelib_back_error_backend;
		notelib_internals_deinit(sio_back_notelib_handle);
		return ret;
	}

	sio_back_sio_outstream->write_callback = write_callback;
	sio_back_sio_outstream->underflow_callback = underflow_callback;

	if(soundio_device_supports_format(sio_back_sio_device, SoundIoFormatFloat32NE)) {
		sio_back_sio_outstream->format = SoundIoFormatFloat32NE;
	} else {
		ret.error.sio = SoundIoErrorIncompatibleDevice;
		soundio_device_unref(sio_back_sio_device);
		sio_back_sio_device = NULL;
		soundio_destroy(sio_back_sio_instance);
		sio_back_sio_instance = NULL;
		ret.error.error_type = notelib_back_error_backend;
		notelib_internals_deinit(sio_back_notelib_handle);
		return ret;
	}

	if((err = soundio_outstream_open(sio_back_sio_outstream))) {
		ret.error.sio = err;
		soundio_outstream_destroy(sio_back_sio_outstream);
		sio_back_sio_outstream = NULL;
		soundio_device_unref(sio_back_sio_device);
		sio_back_sio_device = NULL;
		soundio_destroy(sio_back_sio_instance);
		sio_back_sio_instance = NULL;
		ret.error.error_type = notelib_back_error_backend;
		notelib_internals_deinit(sio_back_notelib_handle);
		return ret;
	}

	if(sio_back_sio_outstream->layout_error)
		fprintf(stderr, "unable to set channel layout: %s\n", soundio_strerror(sio_back_sio_outstream->layout_error));

	if((err = soundio_outstream_start(sio_back_sio_outstream))) {
		ret.error.sio = err;
		soundio_outstream_destroy(sio_back_sio_outstream);
		sio_back_sio_outstream = NULL;
		soundio_device_unref(sio_back_sio_device);
		sio_back_sio_device = NULL;
		soundio_destroy(sio_back_sio_instance);
		sio_back_sio_instance = NULL;
		ret.error.error_type = notelib_back_error_backend;
		notelib_internals_deinit(sio_back_notelib_handle);
		return ret;
	}

	soundio_flush_events(sio_back_sio_instance);

	ret.sample_rate = sio_back_sio_outstream->sample_rate ? sio_back_sio_outstream->sample_rate : 48000;
	ret.error.error_type = notelib_back_error_none;
	return ret;
}
struct notelib_back_error     sio_back_notelib_deinitialize(){
	struct notelib_back_error error;
	error.error_type = notelib_back_error_none;
	soundio_outstream_destroy(sio_back_sio_outstream);
	sio_back_sio_outstream = NULL;
	soundio_device_unref(sio_back_sio_device);
	sio_back_sio_device = NULL;
	soundio_destroy(sio_back_sio_instance);
	sio_back_sio_instance = NULL;
	enum notelib_status ns = notelib_internals_deinit(sio_back_notelib_handle);
	sio_back_notelib_handle = NULL;
	if(ns != notelib_status_ok){
		error.error_type = notelib_back_error_notelib;
		error.notelib = ns;
	}
	return error;
}

#else
#error NO BACKEND SELECTED
#endif


