#include "back.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "notelib/internal.h"
#include "notelib/internal/internals_fwd.h"
#include "notelib/internal/alignment_utilities.h"

#define NOTELIB_BACKEND_ALIGN_PTR_TO_NEXT(PTR, BASE) ((void*)NOTELIB_INTERNAL_ALIGN_TO_NEXT(((uintptr_t)PTR), (BASE)))
#define NOTELIB_BACKEND_ALIGN_PTR_TO_NEXT_ALIGNOF(PTR, BASE) NOTELIB_BACKEND_ALIGN_PTR_TO_NEXT((PTR), (alignof(BASE)))

#define NOTELIB_BACKEND_ALIGN_TO_PREVIOUS(VALUE, BASE) ((VALUE) & ~((BASE)-1))
#define NOTELIB_BACKEND_ALIGN_TO_PREVIOUS_ALIGNOF(VALUE, BASE) NOTELIB_BACKEND_ALIGN_TO_PREVIOUS((VALUE), (alignof(BASE)))

#define NOTELIB_BACKEND_ALIGN_PTR_TO_PREVIOUS(PTR, BASE) ((void*)NOTELIB_BACKEND_ALIGN_TO_PREVIOUS(((uintptr_t)PTR), (BASE)))
#define NOTELIB_BACKEND_ALIGN_PTR_TO_PREVIOUS_ALIGNOF(PTR, BASE) NOTELIB_BACKEND_ALIGN_PTR_TO_PREVIOUS((PTR), (alignof(BASE)))

#define NOTELIB_BACKEND_RETREAT_THEN_ALIGN_TO_PREVIOUS(PTR, RETREAT_OFFSET, ALIGNMENT_BASE) NOTELIB_BACKEND_ALIGN_PTR_TO_PREVIOUS((NOTELIB_INTERNAL_OFFSET_AND_CAST(((unsigned char*)PTR), -(RETREAT_OFFSET), unsigned char*)), (ALIGNMENT_BASE))
#define NOTELIB_BACKEND_RETREAT_THEN_ALIGN_TO_PREVIOUS_ALIGNOF(PTR, RETREAT_OFFSET, ALIGNMENT_BASE) NOTELIB_BACKEND_RETREAT_THEN_ALIGN_TO_PREVIOUS((PTR), (RETREAT_OFFSET), alignof(ALIGNMENT_BASE))
#define NOTELIB_BACKEND_RETREAT_TO_PREVIOUS_OBJECT_OF_TYPE(PTR, TYPE) ((TYPE*)NOTELIB_BACKEND_RETREAT_THEN_ALIGN_TO_PREVIOUS_ALIGNOF((PTR), sizeof(TYPE), alignof(TYPE)))


//NOTE: order must coincide with notelib_backend_info entries
enum notelib_backend_selection{
#if defined(NOTELIB_BACKEND_LIBSOUNDIO) && NOTELIB_BACKEND_LIBSOUNDIO
	notelib_backend_libsoundio,
#endif
#if defined(NOTELIB_BACKEND_PORTAUDIO) && NOTELIB_BACKEND_PORTAUDIO
	notelib_backend_portaudio,
#endif
	notelib_backend_count
};

#if !(defined(NOTELIB_BACKEND_DISABLE) && NOTELIB_BACKEND_DISABLE || (defined(NOTELIB_BACKEND_LIBSOUNDIO) && NOTELIB_BACKEND_LIBSOUNDIO) || (defined(NOTELIB_BACKEND_PORTAUDIO) && NOTELIB_BACKEND_PORTAUDIO))
#warning "No backends provided by notelib's back.c! (If you don't want backends, you can define NOTELIB_BACKEND_DISABLE=1, or just ignore/exclude/delete this file and its header back.h!)"
#endif

//NOTE: order must coincide with notelib_backend_selection entries
NOTELIB_BACKEND_API struct notelib_backend_generic_info notelib_backend_info_impl[notelib_backend_count+1] = {
#if defined(NOTELIB_BACKEND_LIBSOUNDIO) && NOTELIB_BACKEND_LIBSOUNDIO
	{.name = "libsoundio", .version = NULL, .error_description = NULL},
#endif
#if defined(NOTELIB_BACKEND_PORTAUDIO) && NOTELIB_BACKEND_PORTAUDIO
	{.name = "PortAudio", .version = NULL, .error_description = NULL},
#endif
	{.name = NULL, .version = NULL, .error_description = NULL}, //NULL-terminator
};
NOTELIB_BACKEND_API const struct notelib_backend_generic_info* const notelib_backend_info = notelib_backend_info_impl;

struct struct_specs{
	size_t size;
	size_t alignment;
};
#define NOTELIB_STRUCT_SPECS_INIT_FOR(TYPE) {.size = sizeof(TYPE), .alignment = alignof(TYPE)}
static struct struct_specs const empty_struct_specs = {.size = 0, .alignment = 1};

#if defined(NOTELIB_BACKEND_LIBSOUNDIO) && NOTELIB_BACKEND_LIBSOUNDIO
	static struct struct_specs const notelib_backend_libsoundio_specific_data_specs = NOTELIB_STRUCT_SPECS_INIT_FOR(struct notelib_backend_libsoundio_specific_data);

	struct notelib_backend_libsoundio_alloc_init_data{
		struct notelib_backend_libsoundio_init_data init;
		void* hidden_state;
	};

	struct notelib_backend_libsoundio_alloc_init_data notelib_backend_libsoundio_initialize_generic(const struct notelib_params* params);
	struct notelib_backend_libsoundio_error notelib_backend_libsoundio_deinitialize_generic(notelib_state_handle notelib_state);

	struct notelib_backend_generic_audio_info notelib_backend_libsoundio_generic_audio_info(struct notelib_backend_libsoundio_specific_data* backend_state){
		return (struct notelib_backend_generic_audio_info){.sample_rate = backend_state->outstream->sample_rate};
	}
#endif
#if defined(NOTELIB_BACKEND_PORTAUDIO) && NOTELIB_BACKEND_PORTAUDIO
	static struct struct_specs const notelib_backend_portaudio_specific_data_specs = NOTELIB_STRUCT_SPECS_INIT_FOR(struct notelib_backend_portaudio_specific_data);

	struct notelib_backend_portaudio_alloc_init_data{
		struct notelib_backend_portaudio_init_data init;
		void* hidden_state;
	};

	struct notelib_backend_portaudio_alloc_init_data notelib_backend_portaudio_initialize_generic(const struct notelib_params* params);
	struct notelib_backend_portaudio_error notelib_backend_portaudio_deinitialize_generic(notelib_state_handle notelib_state);

	struct notelib_backend_generic_audio_info notelib_backend_portaudio_generic_audio_info(struct notelib_backend_portaudio_specific_data* backend_state){
		const struct PaStreamInfo* const pa_stream_info = Pa_GetStreamInfo(backend_state->stream_handle);
		return (struct notelib_backend_generic_audio_info){.sample_rate = pa_stream_info ? pa_stream_info->sampleRate : 0};
	}
#endif

void* notelib_backend_find_hidden_state_begin(notelib_state_handle notelib_state, struct struct_specs specific_hidden_state_specs);
enum notelib_backend_selection* notelib_backend_find_generic_backend_selection_ptr(notelib_state_handle notelib_state);

bool notelib_backend_begin_padding_required_for_given_hidden_state_alignment(size_t requested_hidden_state_alignment, bool has_generic_backend_selection);
struct struct_specs notelib_backend_specs_for_given_hidden_state(struct struct_specs requested_hidden_state_specs, bool have_generic_backend_selection, bool have_original_ptr);

NOTELIB_BACKEND_API struct notelib_backend_generic_init_data notelib_backend_initialize(const struct notelib_params* params){
	struct notelib_backend_generic_init_data ret;
	#define NOTELIB_BACKEND_INITIALIZE_TRY(BACKEND_NAME, GET_VERSION_STRING, GET_ERROR_STRING, ERROR_INFO_MEMBER)\
		{\
			notelib_backend_info_impl[notelib_backend_##BACKEND_NAME].version = GET_VERSION_STRING;\
			struct notelib_backend_##BACKEND_NAME##_alloc_init_data data = notelib_backend_##BACKEND_NAME##_initialize_generic(params);\
			switch((ret.error_info.type = data.init.error_info.type)){\
				case notelib_backend_error_none:\
					notelib_backend_info_impl[notelib_backend_##BACKEND_NAME].error_description = NULL;\
					ret.notelib_state = data.init.initialized_state.notelib_state;\
					struct struct_specs const requested_struct_specs = notelib_backend_##BACKEND_NAME##_specific_data_specs;\
					struct struct_specs const generic_struct_specs = notelib_backend_specs_for_given_hidden_state(requested_struct_specs, true, notelib_backend_begin_padding_required_for_given_hidden_state_alignment(requested_struct_specs.alignment, true));\
					ret.audio_info = notelib_backend_##BACKEND_NAME##_generic_audio_info(notelib_backend_find_hidden_state_begin(ret.notelib_state, generic_struct_specs));\
					*notelib_backend_find_generic_backend_selection_ptr(data.init.initialized_state.notelib_state) = notelib_backend_##BACKEND_NAME;\
					return ret;\
				case notelib_backend_error_notelib:\
					goto notelib_error;\
				case notelib_backend_error_backend:\
					notelib_backend_info_impl[notelib_backend_##BACKEND_NAME].error_description = GET_ERROR_STRING(data.init.error_info . ERROR_INFO_MEMBER);\
					break;\
				default:\
					return ret;\
			}\
		}

		#if defined(NOTELIB_BACKEND_LIBSOUNDIO) && NOTELIB_BACKEND_LIBSOUNDIO
			NOTELIB_BACKEND_INITIALIZE_TRY(libsoundio, soundio_version_string(), soundio_strerror, sio)
		#endif
		#if defined(NOTELIB_BACKEND_PORTAUDIO) && NOTELIB_BACKEND_PORTAUDIO
			NOTELIB_BACKEND_INITIALIZE_TRY(portaudio, Pa_GetVersionInfo()->versionText, Pa_GetErrorText, pa)
		#endif
	#undef NOTELIB_BACKEND_INITIALIZE_TRY

	return ret;

	notelib_error:
		ret.notelib_state = NULL;
		return ret;
}
NOTELIB_BACKEND_API struct notelib_backend_generic_error notelib_backend_deinitialize(notelib_state_handle notelib_state){
	//notelib_state has to have come from notelib_backend_initialize (no other function!!)
	struct notelib_backend_generic_error ret_error;
	enum notelib_backend_selection selection = *notelib_backend_find_generic_backend_selection_ptr(notelib_state);
	switch(selection){
		#define NOTELIB_BACKEND_DEINITIALIZE_CASE(BACKEND_NAME, GET_ERROR_STRING, ERROR_INFO_MEMBER)\
			case notelib_backend_##BACKEND_NAME:{\
				struct notelib_backend_##BACKEND_NAME##_error error = notelib_backend_##BACKEND_NAME##_deinitialize_generic(notelib_state);\
				switch(ret_error.type = error.type){\
					case notelib_backend_error_notelib:\
						ret_error.notelib = error.notelib;\
						break;\
					case notelib_backend_error_backend:\
						notelib_backend_info_impl[notelib_backend_##BACKEND_NAME].error_description = GET_ERROR_STRING(error . ERROR_INFO_MEMBER);\
						break;\
					default: break;\
				}\
			}break;

			#if defined(NOTELIB_BACKEND_LIBSOUNDIO) && NOTELIB_BACKEND_LIBSOUNDIO
				NOTELIB_BACKEND_DEINITIALIZE_CASE(libsoundio, soundio_strerror, sio)
			#endif
			#if defined(NOTELIB_BACKEND_PORTAUDIO) && NOTELIB_BACKEND_PORTAUDIO
				NOTELIB_BACKEND_DEINITIALIZE_CASE(portaudio, Pa_GetErrorText, pa)
			#endif
		#undef NOTELIB_BACKEND_DEINITIALIZE_CASE

		default:
			fprintf(stderr, "ERROR: unknown (corrupt?) notelib backend (id #%d) in use by notelib_state passed to notelib_backend_deinitialize; was this state created via notelib_backend_initialize?\nFailed to free memory!", selection);
			ret_error.type = notelib_backend_error_notelib;
			ret_error.notelib = notelib_answer_failure_unknown;
	}
	return ret_error;
}

struct any_buffer_slice{
	void* pos;
	size_t length;
};

struct notelib_backend_libsoundio_alloc_init_data notelib_backend_libsoundio_initialize_generic_in(struct any_buffer_slice buffer, const struct notelib_params* params);
struct notelib_backend_libsoundio_error notelib_backend_libsoundio_deinitialize_generic_no_free(notelib_state_handle notelib_state, bool have_original_ptr);
struct notelib_backend_portaudio_alloc_init_data notelib_backend_portaudio_initialize_generic_in(struct any_buffer_slice buffer, const struct notelib_params* params);
struct notelib_backend_portaudio_error notelib_backend_portaudio_deinitialize_generic_no_free(notelib_state_handle notelib_state, bool have_original_ptr);

NOTELIB_BACKEND_API struct notelib_backend_generic_init_data notelib_backend_initialize_in(void* buffer, size_t buffer_size, const struct notelib_params* params){
	struct any_buffer_slice buffer_slice = {.pos = buffer, .length = buffer_size};
	struct notelib_backend_generic_init_data ret;
	#define NOTELIB_BACKEND_INITIALIZE_IN_TRY(BACKEND_NAME, GET_VERSION_STRING, GET_ERROR_STRING, ERROR_INFO_MEMBER)\
		{\
			notelib_backend_info_impl[notelib_backend_##BACKEND_NAME].version = GET_VERSION_STRING;\
			struct notelib_backend_##BACKEND_NAME##_alloc_init_data data = notelib_backend_##BACKEND_NAME##_initialize_generic_in(buffer_slice, params);\
			switch((ret.error_info.type = data.init.error_info.type)){\
				case notelib_backend_error_none:\
					notelib_backend_info_impl[notelib_backend_##BACKEND_NAME].error_description = NULL;\
					ret.notelib_state = data.init.initialized_state.notelib_state;\
					struct struct_specs const requested_struct_specs = notelib_backend_##BACKEND_NAME##_specific_data_specs;\
					struct struct_specs const generic_struct_specs = notelib_backend_specs_for_given_hidden_state(requested_struct_specs, true, false);\
					ret.audio_info = notelib_backend_##BACKEND_NAME##_generic_audio_info(notelib_backend_find_hidden_state_begin(ret.notelib_state, generic_struct_specs));\
					*notelib_backend_find_generic_backend_selection_ptr(data.init.initialized_state.notelib_state) = notelib_backend_##BACKEND_NAME;\
					return ret;\
				case notelib_backend_error_notelib:\
					goto notelib_error;\
				case notelib_backend_error_backend:\
					notelib_backend_info_impl[notelib_backend_##BACKEND_NAME].error_description = GET_ERROR_STRING(data.init.error_info . ERROR_INFO_MEMBER);\
					break;\
				default:\
					return ret;\
			}\
		}

		#if defined(NOTELIB_BACKEND_LIBSOUNDIO) && NOTELIB_BACKEND_LIBSOUNDIO
			NOTELIB_BACKEND_INITIALIZE_IN_TRY(libsoundio, soundio_version_string(), soundio_strerror, sio)
		#endif
		#if defined(NOTELIB_BACKEND_PORTAUDIO) && NOTELIB_BACKEND_PORTAUDIO
			NOTELIB_BACKEND_INITIALIZE_IN_TRY(portaudio, Pa_GetVersionInfo()->versionText, Pa_GetErrorText, pa)
		#endif
	#undef NOTELIB_BACKEND_INITIALIZE_IN_TRY

	return ret;

	notelib_error:
		ret.notelib_state = NULL;
		return ret;
}
NOTELIB_BACKEND_API struct notelib_backend_generic_error notelib_backend_deinitialize_no_free(notelib_state_handle notelib_state){
	//notelib_state has to have come from notelib_backend_initialize (no other function!!)
	struct notelib_backend_generic_error ret_error;
	enum notelib_backend_selection selection = *notelib_backend_find_generic_backend_selection_ptr(notelib_state);
	switch(selection){
		#define NOTELIB_BACKEND_DEINITIALIZE_CASE(BACKEND_NAME, GET_ERROR_STRING, ERROR_INFO_MEMBER)\
			case notelib_backend_##BACKEND_NAME:{\
				struct notelib_backend_##BACKEND_NAME##_error error = notelib_backend_##BACKEND_NAME##_deinitialize_generic_no_free(notelib_state, false);\
				switch(ret_error.type = error.type){\
					case notelib_backend_error_notelib:\
						ret_error.notelib = error.notelib;\
						break;\
					case notelib_backend_error_backend:\
						notelib_backend_info_impl[notelib_backend_##BACKEND_NAME].error_description = GET_ERROR_STRING(error . ERROR_INFO_MEMBER);\
						break;\
					default: break;\
				}\
			}break;

			#if defined(NOTELIB_BACKEND_LIBSOUNDIO) && NOTELIB_BACKEND_LIBSOUNDIO
				NOTELIB_BACKEND_DEINITIALIZE_CASE(libsoundio, soundio_strerror, sio)
			#endif
			#if defined(NOTELIB_BACKEND_PORTAUDIO) && NOTELIB_BACKEND_PORTAUDIO
				NOTELIB_BACKEND_DEINITIALIZE_CASE(portaudio, Pa_GetErrorText, pa)
			#endif
		#undef NOTELIB_BACKEND_DEINITIALIZE_CASE

		default:
			fprintf(stderr, "ERROR: unknown (corrupt?) notelib backend (id #%d) in use by notelib_state passed to notelib_backend_deinitialize_no_free; was this state created via notelib_backend_initialize_in?\nFailed to free memory!", selection);
			ret_error.type = notelib_backend_error_notelib;
			ret_error.notelib = notelib_answer_failure_unknown;
	}
	return ret_error;
}


struct notelib_backend_alloc_hidden_state{
	void* original_ptr;
};
struct notelib_backend_selection_hidden_state{
	enum notelib_backend_selection selection;
};
bool notelib_backend_begin_padding_required_for_given_hidden_state_alignment(size_t alignment, bool has_generic_backend_selection){
	return (alignof(struct notelib_internals) > alignof(max_align_t)) || (alignment > alignof(max_align_t)) || (has_generic_backend_selection && /*note that technically this case is impossible to be the maximum; seriously delete this code*/ (alignof(struct notelib_backend_selection_hidden_state) > alignof(max_align_t)));
}

size_t overalloc_required_for_alignment(size_t base_alignment, size_t required_alignment)
	{return required_alignment > base_alignment ? required_alignment - base_alignment : 0;}
size_t notelib_backend_specific_required_malloc_size_with_hidden_state_alloc(const struct notelib_params* params, struct struct_specs specific_hidden_state_specs){
	size_t max_alignof = MAX(alignof(struct notelib_internals), specific_hidden_state_specs.alignment);
	size_t const max_alignment_overalloc = overalloc_required_for_alignment(alignof(max_align_t), max_alignof);
	return
		NOTELIB_INTERNAL_ALIGN_TO_NEXT(specific_hidden_state_specs.size, alignof(struct notelib_internals))
		+ notelib_internals_size_requirements(params)
		+ max_alignment_overalloc;
}

struct notelib_internals_buffer_slice{
	struct notelib_internals* pos;
	size_t length;
};
struct notelib_backend_internals_hidden_state_positioning_info{
	void* hidden_state;
	struct notelib_internals_buffer_slice internals;
};
struct struct_specs notelib_backend_hidden_state_add_generic_backend_selection_state(struct struct_specs requested_hidden_state_specs){
	return (struct struct_specs){
		.size = NOTELIB_INTERNAL_ALIGN_TO_NEXT_ALIGNOF(requested_hidden_state_specs.size, struct notelib_backend_selection_hidden_state) + sizeof(struct notelib_backend_selection_hidden_state),
		.alignment = MAX(requested_hidden_state_specs.alignment, alignof(struct notelib_backend_selection_hidden_state)),
	};
}
struct struct_specs notelib_backend_hidden_state_add_alloc_hidden_state(struct struct_specs requested_hidden_state_specs){
	return (struct struct_specs){
		.size = NOTELIB_INTERNAL_ALIGN_TO_NEXT_ALIGNOF(requested_hidden_state_specs.size, struct notelib_backend_alloc_hidden_state) + sizeof(struct notelib_backend_alloc_hidden_state),
		.alignment = MAX(requested_hidden_state_specs.alignment, alignof(struct notelib_backend_alloc_hidden_state)),
	};
}
enum notelib_backend_selection* notelib_backend_find_generic_backend_selection_ptr(notelib_state_handle notelib_state){
	return &((NOTELIB_BACKEND_RETREAT_TO_PREVIOUS_OBJECT_OF_TYPE(notelib_state, struct notelib_backend_selection_hidden_state))->selection);
}
void** notelib_backend_find_original_ptr_ptr_of_begin_padded(notelib_state_handle notelib_state, bool has_generic_backend_selection){
	// Note that this function must only be called if malloc alignment was insufficient and required begin padding; otherwise hidden state is at the beginning of the allocated buffer and no original_ptr was / would have been allocated
	void* at_generic_backend_selection = has_generic_backend_selection ? notelib_backend_find_generic_backend_selection_ptr(notelib_state) : notelib_state;
	return &(NOTELIB_BACKEND_RETREAT_TO_PREVIOUS_OBJECT_OF_TYPE(at_generic_backend_selection, struct notelib_backend_alloc_hidden_state)->original_ptr);
}
void* notelib_backend_find_hidden_state_begin(notelib_state_handle notelib_state, struct struct_specs specific_hidden_state_specs){
	return NOTELIB_BACKEND_RETREAT_THEN_ALIGN_TO_PREVIOUS(notelib_state, specific_hidden_state_specs.size, specific_hidden_state_specs.alignment);
}
struct notelib_internals_buffer_slice notelib_backend_realign_internals_buffer_ptr(void* original_buffer_pos, size_t original_buffer_length){
	struct notelib_internals* const notelib_handle = NOTELIB_BACKEND_ALIGN_PTR_TO_NEXT_ALIGNOF(original_buffer_pos, struct notelib_internals);
	return (struct notelib_internals_buffer_slice){
		.pos = notelib_handle,
		.length = original_buffer_length - (((unsigned char*)notelib_handle) - ((unsigned char*)original_buffer_pos))
	};
}
struct notelib_backend_internals_hidden_state_positioning_info notelib_backend_position_internals_with_given_hidden_state_in_buffer(struct any_buffer_slice original_buffer, struct struct_specs specific_hidden_state_specs){
	void* const earliest_hidden_state_position = NOTELIB_BACKEND_ALIGN_PTR_TO_NEXT(original_buffer.pos, specific_hidden_state_specs.alignment);
	void* const internals_position_range_begin = NOTELIB_INTERNAL_OFFSET_AND_CAST(earliest_hidden_state_position, specific_hidden_state_specs.size, void*);
	struct notelib_internals* const internals = NOTELIB_BACKEND_ALIGN_PTR_TO_NEXT_ALIGNOF(internals_position_range_begin, struct notelib_internals);
	size_t const buffer_size_reduction_by_alignment = ((unsigned char*)internals) - ((unsigned char*)original_buffer.pos);
	return (struct notelib_backend_internals_hidden_state_positioning_info){
		.hidden_state = notelib_backend_find_hidden_state_begin(internals, specific_hidden_state_specs),
		.internals = (struct notelib_internals_buffer_slice){
			.pos = original_buffer.length > buffer_size_reduction_by_alignment ? internals : NULL,
			.length = original_buffer.length - buffer_size_reduction_by_alignment,
		},
	};
}
struct struct_specs notelib_backend_specs_for_given_hidden_state(struct struct_specs requested_hidden_state_specs, bool have_generic_backend_selection, bool have_original_pointer_and_begin_padding_required){
	struct struct_specs specific_hidden_state_specs = have_original_pointer_and_begin_padding_required ? notelib_backend_hidden_state_add_alloc_hidden_state(specific_hidden_state_specs) : requested_hidden_state_specs;
	specific_hidden_state_specs = have_generic_backend_selection ? notelib_backend_hidden_state_add_generic_backend_selection_state(requested_hidden_state_specs) : requested_hidden_state_specs;
	return specific_hidden_state_specs;
}
struct notelib_backend_hidden_state_info{
	struct struct_specs specific_specs;
	bool has_generic_backend_selection;
	bool has_original_ptr;
};
struct notelib_backend_internals_hidden_state_setup_info{
	struct any_buffer_slice memory;
	struct notelib_backend_hidden_state_info hidden_state;
};
struct notelib_backend_internals_hidden_state_positioning_info notelib_backend_init_hidden_state_at(struct notelib_backend_internals_hidden_state_setup_info setup_info){
	if(!setup_info.memory.pos)
		goto error_insufficient_memory;
	struct notelib_backend_internals_hidden_state_positioning_info positioning = notelib_backend_position_internals_with_given_hidden_state_in_buffer(setup_info.memory, setup_info.hidden_state.specific_specs);
	if(!positioning.internals.pos)
		goto error_insufficient_memory;

	if(setup_info.hidden_state.specific_specs.size == 0) //if struct of size 0 was requested, trivial operations (like copying its contents) might end up modifying actual memory (sizeof(struct{}) is 1, after all), which we didn't allocate
		positioning.hidden_state = NULL;
	if(setup_info.hidden_state.has_original_ptr)
		*notelib_backend_find_original_ptr_ptr_of_begin_padded(positioning.internals.pos, setup_info.hidden_state.has_generic_backend_selection) = setup_info.memory.pos;

	return positioning;

	error_insufficient_memory:
		return (struct notelib_backend_internals_hidden_state_positioning_info){.hidden_state = NULL, .internals = {.pos = NULL}};
}
struct notelib_backend_hidden_state_info notelib_backend_internals_hidden_state_info_for(struct struct_specs requested_hidden_state_specs, bool have_generic_backend_selection, bool have_original_ptr){
	bool begin_padding_required = have_original_ptr && notelib_backend_begin_padding_required_for_given_hidden_state_alignment(requested_hidden_state_specs.alignment, have_generic_backend_selection);
	return (struct notelib_backend_hidden_state_info){
		.specific_specs = notelib_backend_specs_for_given_hidden_state(requested_hidden_state_specs, have_generic_backend_selection, begin_padding_required),
		.has_generic_backend_selection = have_generic_backend_selection,
		.has_original_ptr = have_original_ptr,
	};
}
struct notelib_backend_internals_hidden_state_positioning_info notelib_backend_init_hidden_state_in_buffer(struct any_buffer_slice original_buffer, struct struct_specs requested_hidden_state_specs, bool have_generic_backend_selection){
	return notelib_backend_init_hidden_state_at((struct notelib_backend_internals_hidden_state_setup_info){
		.memory = original_buffer,
		.hidden_state = notelib_backend_internals_hidden_state_info_for(requested_hidden_state_specs, have_generic_backend_selection, false),
	});
}
struct notelib_backend_internals_hidden_state_positioning_info notelib_backend_alloc_and_init_hidden_state(const struct notelib_params* params, struct struct_specs requested_hidden_state_specs, bool have_generic_backend_selection){
	struct notelib_backend_hidden_state_info const specific_hidden_state_info = notelib_backend_internals_hidden_state_info_for(requested_hidden_state_specs, have_generic_backend_selection, true);
	size_t const buffer_size_required = notelib_backend_specific_required_malloc_size_with_hidden_state_alloc(params, specific_hidden_state_info.specific_specs);
	return notelib_backend_init_hidden_state_at((struct notelib_backend_internals_hidden_state_setup_info){
		.memory = {
			.pos = malloc(buffer_size_required),
			.length = buffer_size_required
		},
		.hidden_state = specific_hidden_state_info,
	});
}
void notelib_backend_dealloc_internals_for_given_hidden_state(notelib_state_handle notelib_state, struct struct_specs requested_hidden_state_specs, bool has_generic_backend_selection){
	bool begin_padding_required = notelib_backend_begin_padding_required_for_given_hidden_state_alignment(requested_hidden_state_specs.alignment, has_generic_backend_selection);
	void* allocated_buffer_begin = begin_padding_required ? *notelib_backend_find_original_ptr_ptr_of_begin_padded(notelib_state, has_generic_backend_selection) :
			notelib_backend_find_hidden_state_begin(notelib_state, notelib_backend_specs_for_given_hidden_state(requested_hidden_state_specs, has_generic_backend_selection, begin_padding_required));
	free(allocated_buffer_begin);
}



static const unsigned int notelib_backend_underflow_prevention_step = 12;

#if defined(NOTELIB_BACKEND_LIBSOUNDIO) && NOTELIB_BACKEND_LIBSOUNDIO


	//makeshift underflow countermeasure
	static unsigned int notelib_backend_libsoundio_underflow_prevention_buffer_size = 0;

	#ifndef NOTELIB_BACKEND_LIBSOUNDIO_ALLOW_POTENTIAL_UNDERFLOW_RACE_CONDITION
	static bool notelib_backend_libsoundio_static_underflow_prevention_already_in_use = false;
	#ifndef NOTELIB_BACKEND_LIBSOUNDIO_STATIC_UNDERFLOW_PREVENTION_ALREADY_IN_USE_ERROR_INFO
		#define NOTELIB_BACKEND_LIBSOUNDIO_STATIC_UNDERFLOW_PREVENTION_ALREADY_IN_USE_ERROR_INFO ((struct notelib_backend_libsoundio_error){.type = notelib_backend_error_backend, .sio = SoundIoErrorBackendUnavailable})
	#endif
	#endif

	static int imin(int a, int b) {return a <= b ? a : b;}
	static int imax(int a, int b) {return a >= b ? a : b;}
	static void write_sample_float32ne(char *ptr, notelib_sample sample) {
		*((float*)ptr) = sample;
	}
	static notelib_sample sio_back_intermediate_audio_buffer[1<<12];
	static int last_max_frame_count = 0;
	static void write_callback(struct SoundIoOutStream *outstream, int frame_count_min, int frame_count_max) {
		notelib_state_handle notelib_handle = outstream->userdata;

		last_max_frame_count = frame_count_max;

		struct SoundIoChannelArea *areas;
		int err;

		int frames_left = imin(imax(frame_count_min, notelib_backend_libsoundio_underflow_prevention_buffer_size), frame_count_max);//min(frame_count_max, max(outstream->sample_rate*.01/* = 10 ms*/, frame_count_min));

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
				notelib_internals_fill_buffer(notelib_handle, sio_back_intermediate_audio_buffer, generated_frames);
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
		if(notelib_backend_libsoundio_underflow_prevention_buffer_size){
			//outstream->write_callback(outstream, imin(underflow_prevention, last_max_frame_count), last_max_frame_count); //worked in testing, not sure if this has any actual benefit
			//alternative prevention strategy: underflow_prevention <<= 1;
		}//alternative prevention strategy: else underflow_prevention = 1;
		notelib_backend_libsoundio_underflow_prevention_buffer_size += notelib_backend_underflow_prevention_step;
		fprintf(stderr, "underflow %d | p -> %d\n", count++, notelib_backend_libsoundio_underflow_prevention_buffer_size);
	}

	static struct notelib_backend_libsoundio_data const notelib_backend_libsoundio_data_empty = {
		.notelib_state = NULL,
		.sio_state = {
			.instance = NULL,
			.device = NULL,
			.outstream = NULL,
		},
	};
	NOTELIB_BACKEND_SPECIFIC_API struct notelib_backend_libsoundio_init_data notelib_backend_libsoundio_initialize_in(void* buffer, size_t buffer_size, const struct notelib_params* params){

		struct notelib_internals_buffer_slice realigned_buffer = notelib_backend_realign_internals_buffer_ptr(buffer, buffer_size);
		struct notelib_backend_libsoundio_init_data ret = {
			.initialized_state = notelib_backend_libsoundio_data_empty,
			.error_info = {.type = notelib_backend_error_none},
		};
		#ifndef NOTELIB_BACKEND_LIBSOUNDIO_ALLOW_POTENTIAL_UNDERFLOW_RACE_CONDITION
			if(notelib_backend_libsoundio_static_underflow_prevention_already_in_use){
				ret.error_info = NOTELIB_BACKEND_LIBSOUNDIO_STATIC_UNDERFLOW_PREVENTION_ALREADY_IN_USE_ERROR_INFO;
				return ret;
			}
		#endif

		ret.initialized_state.notelib_state = realigned_buffer.pos;
		enum notelib_status ns = notelib_internals_init(ret.initialized_state.notelib_state, realigned_buffer.length, params);
		if(ns != notelib_status_ok){
			ret.initialized_state.notelib_state = NULL;
			ret.error_info.type = notelib_backend_error_notelib;
			ret.error_info.notelib = ns;
			goto any_error;
		}

		ret.initialized_state.sio_state.instance = soundio_create();
		if(!ret.initialized_state.sio_state.instance){
			ret.error_info.sio = SoundIoErrorNoMem;
			goto backend_error;
		}

		enum SoundIoError err = soundio_connect(ret.initialized_state.sio_state.instance);
		if(err){
			ret.error_info.sio = err;
			goto backend_error;
		}

		soundio_flush_events(ret.initialized_state.sio_state.instance);

		int selected_device_index = soundio_default_output_device_index(ret.initialized_state.sio_state.instance);
		if(selected_device_index < 0){
			ret.error_info.sio = SoundIoErrorNoSuchDevice;
			goto backend_error;
		}

		ret.initialized_state.sio_state.device = soundio_get_output_device(ret.initialized_state.sio_state.instance, selected_device_index);
		if(!ret.initialized_state.sio_state.device){
			ret.error_info.sio = SoundIoErrorOpeningDevice;
			goto backend_error;
		}

		if((ret.error_info.sio = ret.initialized_state.sio_state.device->probe_error))
			goto backend_error;

		ret.initialized_state.sio_state.outstream = soundio_outstream_create(ret.initialized_state.sio_state.device);
		if(!ret.initialized_state.sio_state.outstream) {
			ret.error_info.sio = SoundIoErrorNoMem;
			goto backend_error;
		}

		ret.initialized_state.sio_state.outstream->userdata = ret.initialized_state.notelib_state;
		ret.initialized_state.sio_state.outstream->write_callback = write_callback;
		ret.initialized_state.sio_state.outstream->underflow_callback = underflow_callback;

		if(soundio_device_supports_format(ret.initialized_state.sio_state.device, SoundIoFormatFloat32NE)){
			ret.initialized_state.sio_state.outstream->format = SoundIoFormatFloat32NE;
		}else{
			ret.error_info.sio = SoundIoErrorIncompatibleDevice;
			goto backend_error;
		}

		if((ret.error_info.sio = soundio_outstream_open(ret.initialized_state.sio_state.outstream)))
			goto backend_error;

		if(ret.initialized_state.sio_state.outstream->layout_error)
			fprintf(stderr, "unable to set channel layout: %s\n", soundio_strerror(ret.initialized_state.sio_state.outstream->layout_error));

		if((ret.error_info.sio = soundio_outstream_start(ret.initialized_state.sio_state.outstream)))
			goto backend_error;

		#ifndef NOTELIB_BACKEND_LIBSOUNDIO_ALLOW_POTENTIAL_UNDERFLOW_RACE_CONDITION
			notelib_backend_libsoundio_static_underflow_prevention_already_in_use = true;
		#endif

		return ret;

		backend_error:
			ret.error_info.type = notelib_backend_error_backend;
		any_error:
			notelib_backend_libsoundio_deinitialize_no_free(&ret.initialized_state);
			return ret;
	}
	NOTELIB_BACKEND_SPECIFIC_API struct notelib_backend_libsoundio_error notelib_backend_libsoundio_deinitialize_no_free(struct notelib_backend_libsoundio_data* backend_state){
		struct notelib_backend_libsoundio_error error = {.type = notelib_backend_error_none};
		if(backend_state->sio_state.outstream){
			soundio_outstream_destroy(backend_state->sio_state.outstream);
			backend_state->sio_state.outstream = NULL;
		}
		if(backend_state->sio_state.device){
			soundio_device_unref(backend_state->sio_state.device);
			backend_state->sio_state.device = NULL;
		}
		if(backend_state->sio_state.instance){
			soundio_destroy(backend_state->sio_state.instance);
			backend_state->sio_state.instance = NULL;
			#ifndef NOTELIB_BACKEND_LIBSOUNDIO_ALLOW_POTENTIAL_UNDERFLOW_RACE_CONDITION
				if(!notelib_backend_libsoundio_static_underflow_prevention_already_in_use)
					fprintf(stderr, "Warning: (corrupt?) libsoundio notelib backend deinitialized, but singleton (static, underflow prevention) data was not marked in use!");
			#endif
		}
		#ifndef NOTELIB_BACKEND_LIBSOUNDIO_ALLOW_POTENTIAL_UNDERFLOW_RACE_CONDITION
			notelib_backend_libsoundio_static_underflow_prevention_already_in_use = false;
		#endif
		if(backend_state->notelib_state){
			enum notelib_status ns = notelib_internals_deinit(backend_state->notelib_state);
			backend_state->notelib_state = NULL;
			if(ns != notelib_status_ok){
				error.type = notelib_backend_error_notelib;
				error.notelib = ns;
			}
		}
		return error;
	}

	struct notelib_backend_libsoundio_alloc_init_data notelib_backend_libsoundio_initialize_at_with_hidden_state(struct notelib_backend_internals_hidden_state_positioning_info positioning, const struct notelib_params* params){
		if(!positioning.internals.pos)
			return (struct notelib_backend_libsoundio_alloc_init_data){
				.init = {
					.initialized_state = notelib_backend_libsoundio_data_empty,
					.error_info = {.type = notelib_backend_error_notelib, .notelib = notelib_answer_failure_bad_alloc}, // more precisely, if setup_info.memory.pos != NULL, it is "insufficient memory provided", otherwise it is "could not allocate required memory"
				},
				.hidden_state = NULL,
			};
		return (struct notelib_backend_libsoundio_alloc_init_data){
			.init = notelib_backend_libsoundio_initialize_in(positioning.internals.pos, positioning.internals.length, params),
			.hidden_state = positioning.hidden_state,
		};
	}
	NOTELIB_BACKEND_SPECIFIC_API struct notelib_backend_libsoundio_init_data notelib_backend_libsoundio_initialize(const struct notelib_params* params){
		return notelib_backend_libsoundio_initialize_at_with_hidden_state(notelib_backend_alloc_and_init_hidden_state(params, empty_struct_specs, false), params).init;
	}
	NOTELIB_BACKEND_SPECIFIC_API struct notelib_backend_libsoundio_error notelib_backend_libsoundio_deinitialize(struct notelib_backend_libsoundio_data* backend_state){
		struct notelib_backend_libsoundio_error error = notelib_backend_libsoundio_deinitialize_no_free(backend_state);
		notelib_backend_dealloc_internals_for_given_hidden_state(backend_state->notelib_state, empty_struct_specs, false);
		return error;
	}

	struct notelib_backend_libsoundio_alloc_init_data notelib_backend_libsoundio_initialize_generic_hidden_data(struct notelib_backend_libsoundio_alloc_init_data initialized_state){
		if(initialized_state.init.error_info.type == notelib_backend_error_none)
			if(initialized_state.hidden_state)
				*((struct notelib_backend_libsoundio_specific_data*)initialized_state.hidden_state) = initialized_state.init.initialized_state.sio_state;
		return initialized_state;
	}
	struct notelib_backend_libsoundio_alloc_init_data notelib_backend_libsoundio_initialize_generic_in(struct any_buffer_slice buffer, const struct notelib_params* params){
		return notelib_backend_libsoundio_initialize_generic_hidden_data(notelib_backend_libsoundio_initialize_at_with_hidden_state(notelib_backend_init_hidden_state_in_buffer(buffer, notelib_backend_libsoundio_specific_data_specs, true), params));
	}
	struct notelib_backend_libsoundio_error notelib_backend_libsoundio_deinitialize_generic_no_free(notelib_state_handle notelib_state, bool have_original_ptr){
		struct notelib_backend_libsoundio_error error = notelib_backend_libsoundio_deinitialize_no_free(&(struct notelib_backend_libsoundio_data){
			.notelib_state = notelib_state,
			.sio_state = *((struct notelib_backend_libsoundio_specific_data*)notelib_backend_find_hidden_state_begin(notelib_state, notelib_backend_internals_hidden_state_info_for(notelib_backend_libsoundio_specific_data_specs, true, have_original_ptr).specific_specs)),
		});
		return error;
	}

	struct notelib_backend_libsoundio_alloc_init_data notelib_backend_libsoundio_initialize_generic(const struct notelib_params* params){
		return notelib_backend_libsoundio_initialize_generic_hidden_data(notelib_backend_libsoundio_initialize_at_with_hidden_state(notelib_backend_alloc_and_init_hidden_state(params, notelib_backend_libsoundio_specific_data_specs, true), params));
	}
	struct notelib_backend_libsoundio_error notelib_backend_libsoundio_deinitialize_generic(notelib_state_handle notelib_state){
		struct notelib_backend_libsoundio_error error = notelib_backend_libsoundio_deinitialize_generic_no_free(notelib_state, true);
		notelib_backend_dealloc_internals_for_given_hidden_state(notelib_state, notelib_backend_libsoundio_specific_data_specs, true);
		return error;
	}


#endif
#if defined(NOTELIB_BACKEND_PORTAUDIO) && NOTELIB_BACKEND_PORTAUDIO


	//TODO: add underflow prevention mechanism equivalent to the one for libsoundio... maybe? I only now just realized that we never actually return less than the maximum frame count in the callback. So we _should always be safe... I think.

	#include "portaudio.h"

	static int pa_back_callback
	(const void *input, void *output, unsigned long frameCount,
	 const PaStreamCallbackTimeInfo* timeInfo,
	 PaStreamCallbackFlags statusFlags,
	 void *userData){
		//if(statusFlags & paOutputUnderflow)
			///*TODO: handle underflow*/;
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
			if(frameCount <= NOTELIB_SAMPLE_UINT_MAX){
				sample_count = frameCount;
				frameCount = 0;
			}else{
				sample_count = NOTELIB_SAMPLE_UINT_MAX;
				frameCount -= NOTELIB_SAMPLE_UINT_MAX;
			}
			notelib_internals_fill_buffer(notelib_state, out, sample_count);
			out += sample_count;
		}
		return paContinue;
	}

	struct notelib_backend_portaudio_data const notelib_backend_portaudio_data_empty = {
		.notelib_state = NULL,
		.pa_state = {.stream_handle = NULL},
	};
	NOTELIB_BACKEND_SPECIFIC_API struct notelib_backend_portaudio_init_data notelib_backend_portaudio_initialize_in(void* buffer, size_t buffer_size, const struct notelib_params* params){
		struct notelib_internals_buffer_slice realigned_buffer = notelib_backend_realign_internals_buffer_ptr(buffer, buffer_size);
		struct notelib_backend_portaudio_init_data ret = {
			.initialized_state = notelib_backend_portaudio_data_empty,
			.error_info = {.type = notelib_backend_error_none},
		};

		ret.initialized_state.notelib_state = realigned_buffer.pos;
		enum notelib_status ns = notelib_internals_init(ret.initialized_state.notelib_state, buffer_size, params);
		if(ns != notelib_status_ok){
			//"inlined" deinit because notelib_backend_portaudio_deinitialize_no_free _always calls Pa_Terminate()
			ret.initialized_state.notelib_state = NULL;
			ret.error_info.notelib = ns;
			ret.error_info.type = notelib_backend_error_notelib;
			return ret;
		}

		PaError pa_error = Pa_Initialize();
		if(pa_error != paNoError){
			//"inlined" deinit because notelib_backend_portaudio_deinitialize_no_free _always calls Pa_Terminate()
			notelib_internals_deinit(ret.initialized_state.notelib_state);
			ret.initialized_state.notelib_state = NULL;
			ret.error_info.pa = pa_error;
			ret.error_info.type = notelib_backend_error_backend;
			return ret;
		}

		PaStreamParameters streamParameters;
		PaDeviceIndex defaultOutputDevice = Pa_GetDefaultOutputDevice();
		if(defaultOutputDevice == paNoDevice){
			ret.error_info.pa = paDeviceUnavailable;
			goto backend_error_after_pa_initialize;
		}
		streamParameters.device = defaultOutputDevice;
		PaDeviceInfo const * const defaultOutputDeviceInfo = Pa_GetDeviceInfo(streamParameters.device);
		streamParameters.channelCount = 1;
		streamParameters.sampleFormat = paInt16;
		streamParameters.suggestedLatency = defaultOutputDeviceInfo->defaultLowOutputLatency;
		streamParameters.hostApiSpecificStreamInfo = NULL;
		pa_error = Pa_OpenStream(&ret.initialized_state.pa_state.stream_handle, NULL, &streamParameters, defaultOutputDeviceInfo->defaultSampleRate, paFramesPerBufferUnspecified, paNoFlag, &pa_back_callback, ret.initialized_state.notelib_state);
		if(pa_error != paNoError){
			ret.initialized_state.pa_state.stream_handle = NULL;
			ret.error_info.pa = pa_error;
			goto backend_error_after_pa_initialize;
		}

		pa_error = Pa_StartStream(ret.initialized_state.pa_state.stream_handle);
		if(pa_error != paNoError){
			ret.error_info.pa = pa_error;
			goto backend_error_after_pa_initialize;
		}

		return ret;

		backend_error_after_pa_initialize:
			ret.error_info.type = notelib_backend_error_backend;
			notelib_backend_portaudio_deinitialize_no_free(&ret.initialized_state);
			return ret;
	}
	NOTELIB_BACKEND_SPECIFIC_API struct notelib_backend_portaudio_error notelib_backend_portaudio_deinitialize_no_free(struct notelib_backend_portaudio_data* backend_state){
		PaError pa_close_stream_error = paNoError;
		struct notelib_backend_portaudio_error error;
		error.type = notelib_backend_error_none;
		#ifndef NOTELIB_BACKEND_PORTAUDIO_CLOSE_STREAM_VIA_PA_TERMINATE
			if(backend_state->pa_state.stream_handle){
				pa_close_stream_error = Pa_CloseStream(backend_state->pa_state.stream_handle);
				backend_state->pa_state.stream_handle = NULL;
			}
		#endif

		PaError pa_error = Pa_Terminate();
		pa_error = pa_error == paNoError ? pa_close_stream_error : pa_error;


		if(backend_state->notelib_state){
			enum notelib_status ns = notelib_internals_deinit(backend_state->notelib_state);
			if(ns != notelib_status_ok){
				error.type = notelib_backend_error_notelib;
				error.notelib = ns;
			}
			backend_state->notelib_state = NULL;
		}

		if(pa_error != paNoError){
			error.type = notelib_backend_error_backend;
			error.pa = pa_error;
		}

		return error;
	}

	struct notelib_backend_portaudio_alloc_init_data notelib_backend_portaudio_initialize_at_with_hidden_state(struct notelib_backend_internals_hidden_state_positioning_info positioning, const struct notelib_params* params){
		if(!positioning.internals.pos)
			return (struct notelib_backend_portaudio_alloc_init_data){
				.init = {
					.initialized_state = notelib_backend_portaudio_data_empty,
					.error_info = {.type = notelib_backend_error_notelib, .notelib = notelib_answer_failure_bad_alloc}, // more precisely, if setup_info.memory.pos != NULL, it is "insufficient memory provided", otherwise it is "could not allocate required memory"
				},
				.hidden_state = NULL,
			};
		return (struct notelib_backend_portaudio_alloc_init_data){
			.init = notelib_backend_portaudio_initialize_in(positioning.internals.pos, positioning.internals.length, params),
			.hidden_state = positioning.hidden_state,
		};
	}
	NOTELIB_BACKEND_SPECIFIC_API struct notelib_backend_portaudio_init_data notelib_backend_portaudio_initialize(const struct notelib_params* params){
		return notelib_backend_portaudio_initialize_at_with_hidden_state(notelib_backend_alloc_and_init_hidden_state(params, empty_struct_specs, false), params).init;
	}
	NOTELIB_BACKEND_SPECIFIC_API struct notelib_backend_portaudio_error notelib_backend_portaudio_deinitialize(struct notelib_backend_portaudio_data* backend_state){
		struct notelib_backend_portaudio_error error = notelib_backend_portaudio_deinitialize_no_free(backend_state);
		notelib_backend_dealloc_internals_for_given_hidden_state(backend_state->notelib_state, empty_struct_specs, false);
		return error;
	}

	struct notelib_backend_portaudio_alloc_init_data notelib_backend_portaudio_initialize_generic_hidden_data(struct notelib_backend_portaudio_alloc_init_data initialized_state){
		if(initialized_state.init.error_info.type == notelib_backend_error_none)
			if(initialized_state.hidden_state)
				*((struct notelib_backend_portaudio_specific_data*)initialized_state.hidden_state) = initialized_state.init.initialized_state.pa_state;
		return initialized_state;
	}
	struct notelib_backend_portaudio_alloc_init_data notelib_backend_portaudio_initialize_generic_in(struct any_buffer_slice buffer, const struct notelib_params* params){
		return notelib_backend_portaudio_initialize_generic_hidden_data(notelib_backend_portaudio_initialize_at_with_hidden_state(notelib_backend_init_hidden_state_in_buffer(buffer, notelib_backend_portaudio_specific_data_specs, true), params));
	}
	struct notelib_backend_portaudio_error notelib_backend_portaudio_deinitialize_generic_no_free(notelib_state_handle notelib_state, bool have_original_ptr){
		struct notelib_backend_portaudio_error error = notelib_backend_portaudio_deinitialize_no_free(&(struct notelib_backend_portaudio_data){
			.notelib_state = notelib_state,
			.pa_state = *((struct notelib_backend_portaudio_specific_data*)notelib_backend_find_hidden_state_begin(notelib_state, notelib_backend_internals_hidden_state_info_for(notelib_backend_portaudio_specific_data_specs, true, have_original_ptr).specific_specs)),
		});
		return error;
	}

	struct notelib_backend_portaudio_alloc_init_data notelib_backend_portaudio_initialize_generic(const struct notelib_params* params){
		return notelib_backend_portaudio_initialize_generic_hidden_data(notelib_backend_portaudio_initialize_at_with_hidden_state(notelib_backend_alloc_and_init_hidden_state(params, notelib_backend_portaudio_specific_data_specs, true), params));
	}
	struct notelib_backend_portaudio_error notelib_backend_portaudio_deinitialize_generic(notelib_state_handle notelib_state){
		struct notelib_backend_portaudio_error error = notelib_backend_portaudio_deinitialize_generic_no_free(notelib_state, true);
		notelib_backend_dealloc_internals_for_given_hidden_state(notelib_state, notelib_backend_portaudio_specific_data_specs, true);
		return error;
	}


#endif
