#ifndef NOTELIB_INTERNAL_H_
#define NOTELIB_INTERNAL_H_

//TODO: (kinda implicit): Tidy everything up! Split different things into their own files and separate function heads from their bodies!

//TODO: Additional feature: triggering specified notelib_processing_step_function-s at specified positions; requires a slight restructuring of the fill_buffer routine and some referencing mechanism (probably id) spawned at play_note
//TODO: Additional feature: global (notelib_internal-level) (and maybe instrument-level) processing-step-style functions for "shader-like" effects etc.
//TODO: (eventually) register exit_handler with atexit

#ifndef NOTELIB_INTERNAL_USE_INTERMEDIATE_MIXING_BUFFER
#define NOTELIB_INTERNAL_USE_INTERMEDIATE_MIXING_BUFFER 1
#endif

#include "notelib.h"

#include <stdalign.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct notelib_command{
	enum notelib_command_type {
		notelib_command_type_note,

		notelib_command_type_reset,
		notelib_command_type_set_tempo
	} type;
	union{
		struct notelib_command_note{
			notelib_instrument_uint instrument_index;
		} note;
		struct notelib_tempo{
			notelib_position position_interval;
			notelib_sample_uint interval;
		} tempo;
	};
	notelib_position position;
};

#define NOTELIB_UTIL_CIRCULAR_BUFFER_DATA_ALIGNMENT alignof(struct notelib_command)

#include "util/circular_buffer.h"

#undef NOTELIB_UTIL_CIRCULAR_BUFFER_DATA_ALIGNMENT

struct notelib_processing_step_entry{
	notelib_processing_step_function step;
	notelib_instrument_state_uint data_offset;
	notelib_instrument_state_uint trigger_data_offset;
	notelib_processing_step_setup_function setup;
	notelib_processing_step_cleanup_function cleanup;
};

#define NOTELIB_INSTRUMENT_STATE_DATA_ALIGNMENT alignof(max_align_t)

#define NOTELIB_INTERNAL_NOTELIB_INSTRUMENT_MEMBERS\
	alignas(NOTELIB_INSTRUMENT_STATE_DATA_ALIGNMENT)\
	_Atomic notelib_channel_uint active_channel_count;\
	notelib_channel_uint channel_count;\
	notelib_instrument_state_uint channel_state_size;\
	notelib_step_uint step_count;

//common type of notelib_instrument_inline_steps and notelib_instrument_external_steps
struct notelib_instrument{
	NOTELIB_INTERNAL_NOTELIB_INSTRUMENT_MEMBERS
	//union of struct notelib_processing_step_entry* steps and struct notelib_processing_step_entry inline_steps[] ; size in no way enforced
	//union of void* external_state and unsigned char inline_state[] ; size in no way enforced
};
//this one is assumed to have the strictest alignment requirement out of all of them
struct notelib_instrument_inline_steps{
	NOTELIB_INTERNAL_NOTELIB_INSTRUMENT_MEMBERS
	struct notelib_processing_step_entry inline_steps[];
	//union of void* external_state and unsigned char inline_state[] at &inline_steps[step_count] ; size in no way enforced
};
struct notelib_instrument_external_steps{
	NOTELIB_INTERNAL_NOTELIB_INSTRUMENT_MEMBERS
	struct notelib_processing_step_entry* external_steps;
	//union of void* external_state and unsigned char inline_state[] ; size in no way enforced
};
struct notelib_instrument_external_steps_inline_state{
	NOTELIB_INTERNAL_NOTELIB_INSTRUMENT_MEMBERS
	struct notelib_processing_step_entry* external_steps;
	/*alignas(NOTELIB_INSTRUMENT_STATE_DATA_ALIGNMENT)*/
	unsigned char inline_state[];
};
struct notelib_instrument_external_steps_external_state{
	NOTELIB_INTERNAL_NOTELIB_INSTRUMENT_MEMBERS
	struct notelib_processing_step_entry* external_steps;
	void* external_state;
};

const size_t notelib_instrument_offsetof_steps_array          = offsetof(struct notelib_instrument_inline_steps,                    inline_steps);
const size_t notelib_instrument_offsetof_steps_ptr            = offsetof(struct notelib_instrument_external_steps,                external_steps);
const size_t notelib_instrument_external_offsetof_state_array = offsetof(struct notelib_instrument_external_steps_inline_state,     inline_state);
const size_t notelib_instrument_external_offsetof_state_ptr   = offsetof(struct notelib_instrument_external_steps_external_state, external_state);

#define MAX(A, B) ((A) > (B) ? (A) : (B))
#define ALIGNAS_MAX(A, B) alignas(MAX(alignof(A), alignof(B)))

struct notelib_track{
	ALIGNAS_MAX
	(struct circular_buffer_liberal_reader_unsynchronized,
	 struct circular_buffer_liberal_reader_unsynchronized*)
	notelib_position tempo_ceil_interval;
	notelib_position position;
	notelib_sample_uint tempo_ceil_interval_samples;
	notelib_sample_uint position_sample_offset;
	uint32_t initialized_channel_buffer_size;
	struct circular_buffer command_queue;
	//union of struct circular_buffer_liberal_reader_unsynchronized inline_initialized_channel_buffer and circular_buffer_liberal_reader_unsynchronized external_initialized_channel_buffer ; size in no way enforced
};

notelib_sample_uint notelib_track_get_tempo_interval(const struct notelib_track* track, notelib_position first_position, notelib_position last_position){
	notelib_position tempo_ceil_interval = track->tempo_ceil_interval;
	notelib_position first_position_aligned = first_position % tempo_ceil_interval;
	notelib_sample_uint tempo_ceil_interval_samples = track->tempo_ceil_interval_samples;
	notelib_sample_uint samples_before_first_position = (((uint64_t)tempo_ceil_interval_samples) * first_position_aligned) / tempo_ceil_interval;
	notelib_position last_position_aligned = last_position % tempo_ceil_interval;
	notelib_sample_uint samples_after_last_position = (((uint64_t)tempo_ceil_interval_samples) * (tempo_ceil_interval - last_position_aligned)) / tempo_ceil_interval;
	notelib_sample_uint remaining_samples = (1 + last_position/tempo_ceil_interval - first_position/tempo_ceil_interval) * tempo_ceil_interval_samples;
	return remaining_samples - samples_before_first_position - samples_after_last_position;
}

//I'm about 85% sure that this implementation is correct. (Keep in mind that (currently) the "odd" samples are always floored, not rounded to nearest.)
notelib_position notelib_track_get_position_change(const struct notelib_track* track, notelib_position starting_position, notelib_sample_uint sample_interval){
	notelib_position maximally_covered_positions = (((uint64_t)sample_interval)*track->tempo_ceil_interval)/track->tempo_ceil_interval_samples + 1; //It couldn't ever be more than 1 gained... right?
	if(sample_interval < notelib_track_get_tempo_interval(track, starting_position, starting_position + maximally_covered_positions))
		return maximally_covered_positions - 1;
	else
		return maximally_covered_positions;
}

const size_t notelib_tracks_offsetof_command_queue = offsetof(struct notelib_track, command_queue);

struct notelib_internals{
	ALIGNAS_MAX(struct notelib_instrument, struct circular_buffer)
	notelib_instrument_uint instrument_count;
	uint8_t inline_step_count;
	uint16_t reserved_inline_state_space;
	uint16_t dual_buffer_size;
	notelib_track_uint track_count;
	uint16_t command_queue_size;
	uint16_t reserved_inline_initialized_channel_buffer_size;
	uint16_t instrument_size; //= cached notelib_internals_sizeof_instrument(...)
	//instruments: instrument_count*instrument_size
	//circular_buffer notelib_command_queue
};

#define NOTELIB_INTERNAL_ALIGN_TO_NEXT(VALUE, BASE) ((((VALUE)-1) & ~((BASE)-1)) + (BASE))
#define NOTELIB_INTERNAL_ALIGN_TO_NEXT_ALIGNOF(VALUE, BASE) NOTELIB_INTERNAL_ALIGN_TO_NEXT((VALUE), (alignof(BASE)))
#define NOTELIB_INTERNAL_PAD_SIZEOF(BASE, NEXT) NOTELIB_INTERNAL_ALIGN_TO_NEXT_ALIGNOF((sizeof(BASE)), NEXT)

#define NOTELIB_INTERNAL_OFFSET_AND_CAST(PTR, OFFSET, DEST_TYPE) ((DEST_TYPE)(((unsigned char*)(PTR)) + (OFFSET)))

const size_t notelib_internals_offsetof_instruments = NOTELIB_INTERNAL_PAD_SIZEOF(struct notelib_internals, struct notelib_instrument);

struct notelib_instrument* notelib_internals_get_instrument(struct notelib_internals* internals, notelib_instrument_uint index){
	return
		NOTELIB_INTERNAL_OFFSET_AND_CAST
		(internals,
		 notelib_internals_offsetof_instruments
		 + index * internals->instrument_size,
		 struct notelib_instrument*);
}

size_t notelib_instrument_get_inline_steps_size(uint8_t inline_step_count){
	return
		inline_step_count
		* NOTELIB_INTERNAL_PAD_SIZEOF
		  (struct notelib_processing_step_entry,
		   struct notelib_processing_step_entry);
}

size_t notelib_internals_sizeof_instrument_inline_steps_up_to_inline_state_data(uint8_t inline_step_count){
	return
		NOTELIB_INTERNAL_ALIGN_TO_NEXT
		(notelib_instrument_offsetof_steps_array
		 + notelib_instrument_get_inline_steps_size(inline_step_count),
		 NOTELIB_INSTRUMENT_STATE_DATA_ALIGNMENT);
}

size_t notelib_internals_sizeof_instrument_external_steps_up_to_inline_state_data(uint8_t inline_step_count){
	return
		NOTELIB_INTERNAL_ALIGN_TO_NEXT
		(notelib_instrument_offsetof_steps_ptr
		 + sizeof(struct notelib_processing_step_entry*),
		 NOTELIB_INSTRUMENT_STATE_DATA_ALIGNMENT);
}

size_t notelib_internals_sizeof_instrument_inline_steps_up_to_external_state_data_ptr(uint8_t inline_step_count){
	return
		NOTELIB_INTERNAL_ALIGN_TO_NEXT_ALIGNOF
		(notelib_instrument_offsetof_steps_array
		 + notelib_instrument_get_inline_steps_size(inline_step_count),
		 void*);
}

size_t notelib_internals_sizeof_instrument_external_steps_up_to_external_state_data_ptr(uint8_t inline_step_count){
	return
		NOTELIB_INTERNAL_ALIGN_TO_NEXT_ALIGNOF
		(notelib_instrument_offsetof_steps_ptr
		 + sizeof(struct notelib_processing_step_entry*),
		 void*);
}

size_t notelib_internals_sizeof_instrument(uint8_t inline_step_count, uint16_t reserved_inline_state_space){
	return
		NOTELIB_INTERNAL_ALIGN_TO_NEXT_ALIGNOF
		(MAX(notelib_internals_sizeof_instrument_inline_steps_up_to_inline_state_data(inline_step_count), sizeof(struct notelib_processing_step_entry*))
		 + MAX(reserved_inline_state_space, sizeof(void*)),
		 struct notelib_instrument);
}

bool notelib_instrument_are_processing_steps_inline(const struct notelib_internals* internals, const struct notelib_instrument* instrument)
	{return instrument->step_count <= internals->inline_step_count;}

struct notelib_processing_step_entry* notelib_instrument_get_processing_steps(const struct notelib_internals* internals, struct notelib_instrument* instrument){
	if(notelib_instrument_are_processing_steps_inline(internals, instrument))
		return ((struct notelib_instrument_inline_steps*)instrument)->inline_steps;
	else
		return ((struct notelib_instrument_external_steps*)instrument)->external_steps;
}

size_t notelib_internals_effective_sizeof_instrument_up_to_inline_state_data(const struct notelib_internals* internals, struct notelib_instrument* instrument){
	if(notelib_instrument_are_processing_steps_inline(internals, instrument))
		return notelib_internals_sizeof_instrument_inline_steps_up_to_inline_state_data  (instrument->step_count);
	else
		return notelib_internals_sizeof_instrument_external_steps_up_to_inline_state_data(instrument->step_count);
}

size_t notelib_internals_effective_sizeof_instrument_up_to_external_state_data_ptr(const struct notelib_internals* internals, struct notelib_instrument* instrument){
	if(notelib_instrument_are_processing_steps_inline(internals, instrument))
		return notelib_internals_sizeof_instrument_inline_steps_up_to_external_state_data_ptr  (instrument->step_count);
	else
		return notelib_internals_sizeof_instrument_external_steps_up_to_external_state_data_ptr(instrument->step_count);
}

size_t notelib_instrument_get_effective_state_data_inline_space(const struct notelib_internals* internals, struct notelib_instrument* instrument){
	size_t a = notelib_internals_sizeof_instrument(internals->inline_step_count, internals->reserved_inline_state_space);
	size_t b = notelib_internals_effective_sizeof_instrument_up_to_inline_state_data(internals, instrument);
//	printf("instrument size:    %d\nSIZEOF INSTRUMENT:  %d\nUP TO INLINE STATE: %d\n", (int)internals->instrument_size, (int)a, (int)b);
	fflush(stdout);
	return a - b;
}

bool notelib_instrument_is_state_data_inline(const struct notelib_internals* internals, struct notelib_instrument* instrument)
	{return instrument->channel_count * instrument->channel_state_size <= notelib_instrument_get_effective_state_data_inline_space(internals, instrument);}

void*  notelib_instrument_get_inline_state_data_after_inline_steps(const struct notelib_internals* internals, struct notelib_instrument* instrument){
	return
		NOTELIB_INTERNAL_OFFSET_AND_CAST
		(instrument,
		 notelib_internals_sizeof_instrument_inline_steps_up_to_inline_state_data(instrument->step_count),
		 void*);
}

void** notelib_instrument_get_external_state_data_after_inline_steps(const struct notelib_internals* internals, struct notelib_instrument* instrument){
	return
		NOTELIB_INTERNAL_OFFSET_AND_CAST
		 (instrument,
		  notelib_internals_sizeof_instrument_inline_steps_up_to_external_state_data_ptr(instrument->step_count),
		  void**);
}

void* notelib_instrument_get_state_data(const struct notelib_internals* internals, struct notelib_instrument* instrument){
	if(notelib_instrument_are_processing_steps_inline(internals, instrument)){
		if(notelib_instrument_is_state_data_inline(internals, instrument)){
//			printf("INLINE STEPS INLINE DATA GET\n");
//			fflush(stdout);
			return  notelib_instrument_get_inline_state_data_after_inline_steps  (internals, instrument);
		}else{
//			printf("INLINE STEPS EXterN DATA GET\n");
//			fflush(stdout);
			return *notelib_instrument_get_external_state_data_after_inline_steps(internals, instrument);
		}
	}else{
		if(notelib_instrument_is_state_data_inline(internals, instrument)){
//			printf("EXTERN STEPS INLINE DATA GET\n");
//			fflush(stdout);
			return ((struct notelib_instrument_external_steps_inline_state*)  instrument)->  inline_state;
		}else{
//			printf("EXTERN STEPS EXTERN DATA GET\n");
//			fflush(stdout);
			return ((struct notelib_instrument_external_steps_external_state*)instrument)->external_state;
		}
	}
}

void** notelib_instrument_get_external_state_data_ptr(const struct notelib_internals* internals, struct notelib_instrument* instrument, bool processing_steps_inline){
	if(processing_steps_inline)
		return notelib_instrument_get_external_state_data_after_inline_steps(internals, instrument);
	else
		return &((struct notelib_instrument_external_steps_external_state*)instrument)->external_state;
}

size_t notelib_internals_offsetof_dual_audio_buffer(notelib_instrument_uint instrument_count, uint16_t instrument_size){
	return
		NOTELIB_INTERNAL_ALIGN_TO_NEXT_ALIGNOF
		(notelib_internals_offsetof_instruments + instrument_count * instrument_size, notelib_sample);
}

notelib_sample* notelib_internals_get_audio_buffer(struct notelib_internals* internals){
	return
		NOTELIB_INTERNAL_OFFSET_AND_CAST
		(internals,
		 notelib_internals_offsetof_dual_audio_buffer(internals->instrument_count, internals->instrument_size),
		 notelib_sample*);
}

size_t notelib_internals_sizeof_dual_audio_buffer(uint16_t dual_buffer_size)
	{return (1 + NOTELIB_INTERNAL_USE_INTERMEDIATE_MIXING_BUFFER)*dual_buffer_size*sizeof(notelib_sample);}

size_t notelib_internals_offsetof_tracks(notelib_instrument_uint instrument_count, uint16_t instrument_size, uint16_t dual_buffer_size){
	return
		NOTELIB_INTERNAL_ALIGN_TO_NEXT_ALIGNOF
		(notelib_internals_offsetof_dual_audio_buffer(instrument_count, instrument_size)
		 + notelib_internals_sizeof_dual_audio_buffer(dual_buffer_size),
		 struct circular_buffer);
}

struct notelib_track* notelib_internals_get_tracks(struct notelib_internals* internals){
	return
		NOTELIB_INTERNAL_OFFSET_AND_CAST
		(internals,
		 notelib_internals_offsetof_tracks(internals->instrument_count, internals->instrument_size, internals->dual_buffer_size),
		 struct notelib_track*);
}

size_t notelib_internals_sizeof_track_command_queue(uint16_t queued_command_count)
	{return circular_buffer_sizeof(sizeof(struct notelib_command), queued_command_count);}

bool notelib_track_is_initialized_channel_buffer_internal(const struct notelib_internals* internals, const struct notelib_track* track_ptr)
	{return track_ptr->initialized_channel_buffer_size <= internals->reserved_inline_initialized_channel_buffer_size;}

struct circular_buffer_liberal_reader_unsynchronized*  notelib_track_get_inline_initialized_channel_buffer(struct notelib_track* track_ptr, uint16_t queued_command_count){
	return
		NOTELIB_INTERNAL_OFFSET_AND_CAST
		(track_ptr,
		 NOTELIB_INTERNAL_ALIGN_TO_NEXT_ALIGNOF(
		  notelib_tracks_offsetof_command_queue
		  + notelib_internals_sizeof_track_command_queue(queued_command_count),
		 struct circular_buffer_liberal_reader_unsynchronized),
		 struct circular_buffer_liberal_reader_unsynchronized*);
}

struct circular_buffer_liberal_reader_unsynchronized** notelib_track_get_external_initialized_channel_buffer_ptr(struct notelib_track* track_ptr, uint16_t queued_command_count){
	return
		NOTELIB_INTERNAL_OFFSET_AND_CAST
		(track_ptr,
		 NOTELIB_INTERNAL_ALIGN_TO_NEXT_ALIGNOF(
		  notelib_tracks_offsetof_command_queue
		  + notelib_internals_sizeof_track_command_queue(queued_command_count),
		 struct circular_buffer_liberal_reader_unsynchronized*),
		 struct circular_buffer_liberal_reader_unsynchronized**);
}

struct circular_buffer_liberal_reader_unsynchronized* notelib_track_get_initialized_channel_buffer_ptr(const struct notelib_internals* internals, struct notelib_track* track_ptr){
	if(notelib_track_is_initialized_channel_buffer_internal(internals, track_ptr))
		return  notelib_track_get_inline_initialized_channel_buffer      (track_ptr, internals->command_queue_size);
	else
		return *notelib_track_get_external_initialized_channel_buffer_ptr(track_ptr, internals->command_queue_size);
}

size_t notelib_internals_sizeof_track_initialized_channel_buffer(uint16_t initialized_channel_buffer_size)
	{return circular_buffer_liberal_reader_unsynchronized_sizeof(initialized_channel_buffer_size);}

size_t notelib_internals_sizeof_track(uint16_t queued_command_count, uint16_t reserved_inline_initialized_channel_buffer_size){
	return
		NOTELIB_INTERNAL_ALIGN_TO_NEXT_ALIGNOF
		(NOTELIB_INTERNAL_ALIGN_TO_NEXT_ALIGNOF
		 (notelib_tracks_offsetof_command_queue + notelib_internals_sizeof_track_command_queue(queued_command_count),
		  struct circular_buffer_liberal_reader_unsynchronized)
		 + notelib_internals_sizeof_track_initialized_channel_buffer(reserved_inline_initialized_channel_buffer_size),
		 struct notelib_track);
}

struct notelib_track* notelib_internals_get_track(struct notelib_internals* internals, notelib_track_uint track_index){
	return
		NOTELIB_INTERNAL_OFFSET_AND_CAST
		(notelib_internals_get_tracks(internals),
		 track_index * notelib_internals_sizeof_track(internals->command_queue_size, internals->reserved_inline_initialized_channel_buffer_size),
		 struct notelib_track*);
}

size_t notelib_internals_size_requirements(const struct notelib_params* params){
	return
		notelib_internals_offsetof_tracks
		(params->instrument_count,
		 notelib_internals_sizeof_instrument(params->inline_step_count, params->reserved_inline_state_space),
		 params->internal_dual_buffer_size)
		+ params->track_count
		  * notelib_internals_sizeof_track
		    (params->queued_command_count+1, //adding 1 because of the design flaw that one buffer element is always introduced
		     params->reserved_inline_initialized_channel_buffer_size + (params->reserved_inline_initialized_channel_buffer_size>0)); //adding 1 because of the design flaw that one buffer element is always introduced
}

#define MIN(A, B) ((A) < (B) ? (A) : (B))

enum notelib_status notelib_internals_init(void* position, size_t space_available, const struct notelib_params* params){
	if(space_available < notelib_internals_size_requirements(params))
		return notelib_answer_failure_unknown;
	struct notelib_internals* internals = position;
	notelib_instrument_uint instrument_count = params->instrument_count;
	internals->instrument_count  = instrument_count;
	internals->inline_step_count = params->inline_step_count;
	internals->reserved_inline_state_space = params->reserved_inline_state_space;
	uint16_t dual_buffer_size = params->internal_dual_buffer_size;
	internals->dual_buffer_size = dual_buffer_size;
	notelib_track_uint track_count = params->track_count;
	internals->track_count = track_count;
	uint16_t queued_command_count = params->queued_command_count+1; //adding 1 because of the design flaw that one buffer element is always introduced
	internals->command_queue_size = queued_command_count;
	uint16_t reserved_inline_initialized_channel_buffer_size = params->reserved_inline_initialized_channel_buffer_size;
	reserved_inline_initialized_channel_buffer_size += reserved_inline_initialized_channel_buffer_size > 0; //adding 1 because of the design flaw that one buffer element is always introduced
	internals->reserved_inline_initialized_channel_buffer_size = reserved_inline_initialized_channel_buffer_size;
	internals->instrument_size = notelib_internals_sizeof_instrument(params->inline_step_count, params->reserved_inline_state_space);

	for(notelib_instrument_uint instrument_index = 0; instrument_index < instrument_count; ++instrument_index)
		notelib_internals_get_instrument(internals, instrument_index)->step_count = 0;

	size_t sizeof_track = notelib_internals_sizeof_track(queued_command_count, reserved_inline_initialized_channel_buffer_size);
	if(sizeof_track > (space_available - notelib_internals_offsetof_tracks(instrument_count, internals->instrument_size, dual_buffer_size))/track_count)
		return notelib_answer_failure_unknown;
	for(notelib_track_uint track_index = 0; track_index < track_count; ++track_index)
		notelib_internals_get_track(internals, track_index)->tempo_ceil_interval_samples = 0;

	return notelib_answer_success;
}

enum notelib_status notelib_deinit(notelib_state_handle state_handle){
	struct notelib_internals* internals = (struct notelib_internals*)state_handle;
	notelib_track_uint track_count = internals->track_count;
	for(notelib_track_uint track_index = 0; track_index < track_count; ++track_index){
		struct notelib_track* track_ptr = notelib_internals_get_track(internals, track_index);
		if(!notelib_track_is_initialized_channel_buffer_internal(internals, track_ptr))
			free(notelib_track_get_external_initialized_channel_buffer_ptr(track_ptr, internals->command_queue_size));
		track_ptr->tempo_ceil_interval_samples = 0;
	}
	notelib_instrument_uint instrument_count = internals->instrument_count;
	for(notelib_instrument_uint instrument_index = 0; instrument_index < instrument_count; ++instrument_index){
		struct notelib_instrument* instrument = notelib_internals_get_instrument(internals, instrument_index);
		notelib_step_uint step_count = instrument->step_count;
		if(step_count == 0)
			continue;
		struct notelib_processing_step_entry* steps = notelib_instrument_get_processing_steps(internals, instrument);
		notelib_channel_uint active_channel_count = atomic_load_explicit(&instrument->active_channel_count, memory_order_relaxed);
		void* channel_data = notelib_instrument_get_state_data(internals, instrument);
		for(notelib_channel_uint channel_index = 0; channel_index < active_channel_count; ++channel_index){
			void* current_channel_data = NOTELIB_INTERNAL_OFFSET_AND_CAST(channel_data, channel_index*instrument->channel_state_size, void*);
			for(notelib_step_uint step_index = 0; step_index < step_count; ++step_index){
				struct notelib_processing_step_entry* step = steps + step_index;
				notelib_processing_step_cleanup_function cleanup = step->cleanup;
				if(cleanup != NULL)
					cleanup(NOTELIB_INTERNAL_OFFSET_AND_CAST(current_channel_data, step->data_offset, void*));
			}
		}
		if(!notelib_instrument_is_state_data_inline(internals, instrument))
			free(channel_data);
		if(!notelib_instrument_are_processing_steps_inline(internals, instrument))
			free(steps);
		atomic_store_explicit(&instrument->active_channel_count, 0, memory_order_release);
		instrument->step_count = 0;
	}
	return notelib_status_ok;
}

void notelib_internals_execute_instrument_steps
(void* channel_state_front, void* channel_state_back, notelib_instrument_state_uint channel_state_size,
 notelib_step_uint step_count, struct notelib_processing_step_entry* steps,
 notelib_sample* instrument_mix_buffer, notelib_sample* channel_mix_buffer, notelib_sample_uint samples_requested,
 notelib_channel_uint* active_channel_count_ptr){
	bool forward = true;
	bool next_forward = true;
	goto channel_state_iter_skip_entry;
	do{
		void* current_channel_state;
		if(forward){
			channel_state_front = NOTELIB_INTERNAL_OFFSET_AND_CAST(channel_state_front,  channel_state_size, void*);
		channel_state_iter_skip_entry:
			current_channel_state = channel_state_front;
		}else{
			channel_state_back  = NOTELIB_INTERNAL_OFFSET_AND_CAST(channel_state_back,  -channel_state_size, void*);
			current_channel_state = channel_state_back;
		}
		forward = next_forward;
//		printf("ITER\n   %p ;\n   %p\n", channel_state_front, channel_state_back);
		struct notelib_processing_step_entry* current_step = steps;
		notelib_sample_uint produced_samples = current_step->step(NULL, channel_mix_buffer, samples_requested, NOTELIB_INTERNAL_OFFSET_AND_CAST(current_channel_state, current_step->data_offset, void*));
		for(notelib_step_uint i = 1; i < step_count; ++i){
			++current_step;
			notelib_sample_uint newly_produced_samples = current_step->step(channel_mix_buffer, channel_mix_buffer, produced_samples, NOTELIB_INTERNAL_OFFSET_AND_CAST(current_channel_state, current_step->data_offset, void*));
			produced_samples = MIN(produced_samples, newly_produced_samples);
		}
		for(notelib_sample_uint sample_index = 0; sample_index < produced_samples; ++sample_index)
			instrument_mix_buffer[sample_index] += channel_mix_buffer[sample_index];
		if(produced_samples < samples_requested){
			--*active_channel_count_ptr;
			for(notelib_step_uint step_index = 0; step_index < step_count; ++step_index){
				current_step = steps + step_index;
				notelib_processing_step_cleanup_function cleanup = current_step->cleanup;
				if(cleanup != NULL)
					cleanup(NOTELIB_INTERNAL_OFFSET_AND_CAST(current_channel_state, current_step->data_offset, void*));
			}
			next_forward = false;
		}else if(!forward){
			memcpy(channel_state_front, channel_state_back, channel_state_size);
			void* channel_state_back  = NOTELIB_INTERNAL_OFFSET_AND_CAST(channel_state_back, -channel_state_size, void*);
			forward = true;
			next_forward = true;
		}
	}while(channel_state_front != channel_state_back);
}

void cautiously_advance_track_position
(struct notelib_track* track_ptr, notelib_position* new_position, notelib_sample_uint* new_sample_offset, notelib_position old_position, notelib_sample_uint old_position_sample_offset,
 bool* should_check_for_updates, notelib_sample_uint samples_to_process_commands_for){
//	printf("!1 %d\n", (int)*new_sample_offset);
	*new_sample_offset = old_position_sample_offset + samples_to_process_commands_for;
//	printf("!2 %d\n", (int)*new_sample_offset);
//	printf("ADVANCING POS\n  OLS: %d\n  OLD: %d\n  SPC: %d\n", (int)old_position_sample_offset, (int)old_position, (int)samples_to_process_commands_for);
	notelib_sample_uint tempo_interval = notelib_track_get_tempo_interval(track_ptr, old_position, old_position+1);
//	printf("  -TI: %d\n", (int)tempo_interval);
	if(*new_sample_offset >= tempo_interval){
		notelib_position a = notelib_track_get_position_change(track_ptr, old_position, *new_sample_offset);
		*new_position = old_position + a;
		notelib_sample_uint b = notelib_track_get_tempo_interval(track_ptr, old_position, *new_position);
//		printf("!! %d\n", (int)*new_sample_offset);
		*new_sample_offset -= b;
//		printf("!3 %d\n", (int)*new_sample_offset);
//		printf("  NWS: %d\n  NEW: %d\n", (int)*new_sample_offset, (int)*new_position);
//		printf("a=%d\nb=%d\n", (int)a, (int)b);
		track_ptr->position_sample_offset = *new_sample_offset;
		track_ptr->position = *new_position;
		*should_check_for_updates = true;
	}else;
//		printf("  NWS: %d\n", (int)*new_sample_offset);
//	printf("HERE NOW\n");
//	fflush(stdout);
	track_ptr->position_sample_offset = *new_sample_offset;
}

//TODO: Optimize array copies through assignments to memcpy (perhaps via macro)
void notelib_internals_fill_buffer_part(struct notelib_internals* internals, notelib_sample* out, notelib_sample_uint samples_requested, notelib_channel_uint* still_active_channel_count){
	notelib_instrument_uint instrument_count = internals->instrument_count;
#if NOTELIB_INTERNAL_USE_INTERMEDIATE_MIXING_BUFFER
	notelib_sample* instrument_mix_buffer = notelib_internals_get_audio_buffer(internals);
#else//#if NOTELIB_INTERNAL_USE_INTERMEDIATE_MIXING_BUFFER
	notelib_sample* instrument_mix_buffer = out;
#endif//#if NOTELIB_INTERNAL_USE_INTERMEDIATE_MIXING_BUFFER
	uint16_t dual_buffer_size = internals->dual_buffer_size;
	for(uint16_t i = 0; i < dual_buffer_size; ++i)
		instrument_mix_buffer[i] = 0;
#if NOTELIB_INTERNAL_USE_INTERMEDIATE_MIXING_BUFFER
	notelib_sample* channel_mix_buffer = instrument_mix_buffer + internals->dual_buffer_size;
#else//#if NOTELIB_INTERNAL_USE_INTERMEDIATE_MIXING_BUFFER
	notelib_sample* channel_mix_buffer = notelib_internals_get_audio_buffer(internals);
#endif//#if NOTELIB_INTERNAL_USE_INTERMEDIATE_MIXING_BUFFER
	for(notelib_instrument_uint instrument_index = 0; instrument_index < instrument_count; ++instrument_index){
		struct notelib_instrument* instrument_ptr = notelib_internals_get_instrument(internals, instrument_index);
		notelib_step_uint step_count = instrument_ptr->step_count;
		if(step_count > 0){
			notelib_channel_uint active_channel_count = still_active_channel_count[instrument_index];
			if(active_channel_count > 0){
				struct notelib_processing_step_entry* steps = notelib_instrument_get_processing_steps(internals, instrument_ptr);
				notelib_instrument_state_uint channel_state_size = instrument_ptr->channel_state_size;
				void* channel_state_front = notelib_instrument_get_state_data(internals, instrument_ptr);
				void* channel_state_back  = NOTELIB_INTERNAL_OFFSET_AND_CAST(channel_state_front, (active_channel_count - 1) * channel_state_size, void*);
				notelib_internals_execute_instrument_steps
				(channel_state_front, channel_state_back, channel_state_size,
				 step_count, steps,
				 instrument_mix_buffer, channel_mix_buffer, samples_requested,
				 still_active_channel_count + instrument_index);
			}
		}
	}
	notelib_track_uint track_count = internals->track_count;
	for(notelib_track_uint track_index = 0; track_index < track_count; ++track_index){
		notelib_sample_uint samples_to_process_commands_for = samples_requested;
//		printf("CHECKCHEQUE: %d\n", (int)samples_to_process_commands_for);
		struct notelib_track* track_ptr = notelib_internals_get_track(internals, track_index);
		if(track_ptr->tempo_ceil_interval_samples == 0)
			continue;
		bool should_check_for_updates = false;
		notelib_position old_position = track_ptr->position;
		notelib_position new_position = old_position;
		notelib_sample_uint old_position_sample_offset = track_ptr->position_sample_offset;
		notelib_sample_uint new_sample_offset;
//		printf("CHECKCHECK OLS: %d\n", (int)old_position_sample_offset);
		cautiously_advance_track_position(track_ptr, &new_position, &new_sample_offset, old_position, old_position_sample_offset, &should_check_for_updates, samples_to_process_commands_for);
		if(should_check_for_updates){
			struct circular_buffer* command_queue_ptr = &track_ptr->command_queue;
			const struct notelib_command* command_ptr;
			do{
				command_ptr = circular_buffer_direct_read_commence(command_queue_ptr);
			if(command_ptr == NULL) break;
//				printf("PTRC. %p\n", (void*)command_ptr);
				notelib_position ceiled_old_position = old_position + (old_position_sample_offset > 0);
				notelib_position new_position_ceiled = new_position + (new_sample_offset > 0);
				notelib_position command_position = command_ptr->position;
//				printf("POS\n  CMD:    %d\n  CLDOLD: %d\n  NEW   : %d\n", (int)command_position, (int)ceiled_old_position, (int)new_position);
				if(command_position >= ceiled_old_position && command_position < new_position_ceiled){
//					printf("ACCEPTED\n");
					switch(command_ptr->type){
					case notelib_command_type_note:;
						notelib_instrument_uint instrument_index = command_ptr->note.instrument_index;
						struct notelib_instrument* instrument = notelib_internals_get_instrument(internals, instrument_index);
						struct circular_buffer_liberal_reader_unsynchronized* initialized_channel_state_buffer =
							notelib_track_get_initialized_channel_buffer_ptr(internals, track_ptr);
						notelib_instrument_state_uint channel_state_size = instrument->channel_state_size;
						void* instrument_state_data = notelib_instrument_get_state_data(internals, instrument);
						notelib_channel_uint* specific_still_active_channel_count = still_active_channel_count + instrument_index;
						notelib_channel_uint last_channel_index = *specific_still_active_channel_count;
						++*specific_still_active_channel_count;
						void* channel_state_ptr =
							NOTELIB_INTERNAL_OFFSET_AND_CAST
							(instrument_state_data,
							 last_channel_index*channel_state_size,
							 void*);
						circular_buffer_liberal_reader_unsynchronized_read
						(initialized_channel_state_buffer,
						 channel_state_size,
						 channel_state_ptr);
						notelib_sample_uint samples_leftover = new_sample_offset + notelib_track_get_tempo_interval(track_ptr, command_position, new_position);
						notelib_sample_uint samples_skipped = samples_requested - samples_leftover;
						notelib_internals_execute_instrument_steps
						(channel_state_ptr, channel_state_ptr, channel_state_size,
						 instrument->step_count, notelib_instrument_get_processing_steps(internals, instrument),
						 instrument_mix_buffer + samples_skipped, channel_mix_buffer + samples_skipped, samples_leftover,
						  specific_still_active_channel_count);
						break;
					case notelib_command_type_reset:
						old_position = 0;
						old_position_sample_offset = 0;
						new_sample_offset = notelib_track_get_tempo_interval(track_ptr, command_position, new_position);
						new_position = notelib_track_get_position_change(track_ptr, old_position, new_sample_offset);
						new_sample_offset -= notelib_track_get_tempo_interval(track_ptr, old_position, new_position);
						track_ptr->position_sample_offset = new_sample_offset;
						track_ptr->position = new_position;
						break;
					case notelib_command_type_set_tempo:;
						notelib_sample_uint overlap = new_sample_offset + notelib_track_get_tempo_interval(track_ptr, command_position, new_position);
						old_position = command_position;
						old_position_sample_offset = 0;
						const struct notelib_tempo* command_tempo = &command_ptr->tempo;
						notelib_position command_tempo_ceil_interval = command_tempo->position_interval;
						track_ptr->tempo_ceil_interval = command_tempo_ceil_interval;
						if(command_tempo_ceil_interval == 0)
							fputs("WARNING (notelib): Received halting set tempo command! (Ignoring...)\n", stderr);
						else{
							track_ptr->tempo_ceil_interval_samples = command_tempo->interval;
							new_position = old_position + notelib_track_get_position_change(track_ptr, old_position, overlap);
							new_sample_offset = overlap - notelib_track_get_tempo_interval(track_ptr, old_position, new_position);
						}
						track_ptr->position_sample_offset = new_sample_offset;
						track_ptr->position = new_position;
						break;
					}
					circular_buffer_direct_read_commit(command_queue_ptr);
				}else
					break;
			}while(true);
		}
	}
#if NOTELIB_INTERNAL_USE_INTERMEDIATE_MIXING_BUFFER
	for(notelib_sample_uint sample_index = 0; sample_index < samples_requested; ++sample_index)
		out[sample_index] = instrument_mix_buffer[sample_index];
#endif//#if NOTELIB_INTERNAL_USE_INTERMEDIATE_MIXING_BUFFER
}

void notelib_internals_fill_buffer(struct notelib_internals* internals, notelib_sample* out, notelib_sample_uint samples_required){
	notelib_instrument_uint instrument_count = internals->instrument_count;
	//This VLA is a slight memory concern;
	//being only 255 entries maximum most stacks should be able to handle it (and if you're using that many concurrent instruments you should be able to spare it anyway),
	//but it might still be worth it adding a cache variable to struct notelib_isntrument directly.
	notelib_channel_uint still_active_channel_count[instrument_count];
	for(notelib_instrument_uint instrument_index = 0; instrument_index < instrument_count; ++instrument_index){
		struct notelib_instrument* instrument_ptr = notelib_internals_get_instrument(internals, instrument_index);
		if(instrument_ptr->step_count > 0)
			still_active_channel_count[instrument_index] = atomic_load_explicit(&instrument_ptr->active_channel_count, memory_order_relaxed);
	}
	notelib_sample_uint audio_buffer_size = internals->dual_buffer_size;
	if(audio_buffer_size == 0)
		return; //all hope lost, lol
	notelib_sample_uint full_passes     = samples_required / audio_buffer_size;
	notelib_sample_uint final_pass_size = samples_required % audio_buffer_size;
	for(notelib_sample_uint pass = 0; pass < full_passes; ++pass){
		notelib_internals_fill_buffer_part(internals, out, audio_buffer_size, still_active_channel_count);
		out += audio_buffer_size;
	}
	if(final_pass_size > 0)
		notelib_internals_fill_buffer_part(internals, out, final_pass_size, still_active_channel_count);
	for(notelib_instrument_uint instrument_index = 0; instrument_index < instrument_count; ++instrument_index)
		atomic_store_explicit(&notelib_internals_get_instrument(internals, instrument_index)->active_channel_count, still_active_channel_count[instrument_index], memory_order_release);
}

//could possibly optimize to single realloc call in certain cases
enum notelib_status notelib_register_instrument
(notelib_state_handle notelib_state,
 notelib_instrument_uint* instrument_index_dest,
 notelib_step_uint step_count,
 const struct notelib_processing_step_spec* steps,
 notelib_channel_uint channel_count){
	struct notelib_instrument* instrument;
	for(notelib_instrument_uint instrument_index = 0; instrument_index < ((struct notelib_internals*)notelib_state)->instrument_count; ++instrument_index){
		instrument = notelib_internals_get_instrument(notelib_state, instrument_index);
		if(instrument->step_count == 0){
			*instrument_index_dest = instrument_index;
			goto instrument_index_found;
		}
	}
	return notelib_answer_failure_unknown;
instrument_index_found:
	atomic_store_explicit(&instrument->active_channel_count, 0, memory_order_relaxed);
	instrument->step_count = step_count;
	struct notelib_processing_step_entry* steps_ptr;
	void* steps_ptr_to_free_at_failure;
	bool processing_steps_inline = notelib_instrument_are_processing_steps_inline(notelib_state, instrument);
	if(processing_steps_inline){
		steps_ptr = ((struct notelib_instrument_inline_steps*)instrument)->inline_steps;
		steps_ptr_to_free_at_failure = NULL;
	}else{
		steps_ptr = malloc(step_count*(sizeof(struct notelib_processing_step_entry)));
		if(steps_ptr == NULL){
			instrument->step_count = 0;
			return notelib_answer_failure_unknown;
		}
		steps_ptr_to_free_at_failure = steps_ptr;
		((struct notelib_instrument_external_steps*)instrument)->external_steps = steps_ptr;
	}
	notelib_instrument_state_uint state_offset = 0;
	for(notelib_step_uint step_index = 0; step_index < step_count; ++step_index){
		steps_ptr[step_index].step = steps[step_index].step;
		steps_ptr[step_index].data_offset = state_offset;
		state_offset += steps[step_index].state_data_size;
		steps_ptr[step_index].trigger_data_offset = steps[step_index].trigger_data_offset;
		steps_ptr[step_index].setup = steps[step_index].setup;
		steps_ptr[step_index].cleanup = steps[step_index].cleanup;
	}
//	printf("channel_state_size: %d\n", (int)state_offset);
	fflush(stdout);
	instrument->channel_state_size = state_offset;
//	printf("channel count:      %d\n", (int)channel_count);
	fflush(stdout);
	instrument->channel_count = channel_count;
	if(!notelib_instrument_is_state_data_inline(notelib_state, instrument)){
//		printf("channels not inline\n");
		fflush(stdout);
		void* state_data = malloc(channel_count * state_offset);
		if(state_data == NULL){
			//if(steps_ptr_to_free_at_failure != NULL)
			free(steps_ptr_to_free_at_failure);
			instrument->step_count = 0;
			return notelib_answer_failure_unknown;
		}
		*notelib_instrument_get_external_state_data_ptr(notelib_state, instrument, processing_steps_inline) = state_data;
	}else{
//		printf("channels inline\n");
//		fflush(stdout);
	}
	return notelib_answer_success;
}
//could possibly optimize to single realloc call in certain cases
enum notelib_status notelib_set_instrument_channel_count(notelib_state_handle notelib_state, notelib_instrument_uint instrument_index, notelib_channel_uint channel_count){
	struct notelib_instrument* instrument_ptr = notelib_internals_get_instrument(notelib_state, instrument_index);
	if(atomic_load_explicit(&instrument_ptr->active_channel_count, memory_order_acquire) != 0) //Would optimally not actually be required if the user is careful enough
		return notelib_answer_failure_unknown;
	//Another failure would be to have a note command in the command queue (which we can't check for here) and not push any new commands before it is activated (missing the synchronizing read).
	bool processing_steps_inline = notelib_instrument_are_processing_steps_inline(notelib_state, instrument_ptr);
	void* ptr_to_free;
	if(!notelib_instrument_is_state_data_inline(notelib_state, instrument_ptr))
		ptr_to_free = *notelib_instrument_get_external_state_data_ptr(notelib_state, instrument_ptr, processing_steps_inline);
	else
		ptr_to_free = NULL;
	notelib_channel_uint channel_count_before = instrument_ptr->channel_count;
	instrument_ptr->channel_count = channel_count;
	if(!notelib_instrument_is_state_data_inline(notelib_state, instrument_ptr)){
		void* state_data = malloc(channel_count * instrument_ptr->channel_state_size);
		if(state_data == NULL){
			instrument_ptr->channel_count = channel_count_before;
			return notelib_answer_failure_unknown;
		}
		*notelib_instrument_get_external_state_data_ptr(notelib_state, instrument_ptr, processing_steps_inline) = state_data;
	}
	//if(ptr_to_free != NULL)
	free(ptr_to_free);
	return notelib_answer_success;
}
enum notelib_status notelib_unregister_instrument(notelib_state_handle notelib_state, notelib_instrument_uint instrument_index){
	struct notelib_instrument* instrument_ptr = notelib_internals_get_instrument(notelib_state, instrument_index);
	if(atomic_load_explicit(&instrument_ptr->active_channel_count, memory_order_acquire) != 0) //Would optimally not actually be required if the user is careful enough
		return notelib_answer_failure_unknown;
	//Another failure would be to have a note command in the command queue (which we can't check for here), unregister the instrument and not register another one on time.
	bool processing_steps_inline = notelib_instrument_are_processing_steps_inline(notelib_state, instrument_ptr);
	if(!processing_steps_inline)
		free(((struct notelib_instrument_external_steps*)instrument_ptr)->external_steps);
	if(!notelib_instrument_is_state_data_inline(notelib_state, instrument_ptr))
		free(*notelib_instrument_get_external_state_data_ptr(notelib_state, instrument_ptr, processing_steps_inline));
	instrument_ptr->step_count = 0;
	return notelib_answer_success;
}

//TODO: For concurrency to be safe there has to be some sort of trigger for starting to query a track coming from the client thread. Implement that somehow!
enum notelib_status notelib_start_track
(notelib_state_handle notelib_state, notelib_track_uint* track_index_dest, uint32_t initialized_channel_buffer_size, notelib_position tempo_interval, notelib_sample_uint tempo_interval_samples){
	if(tempo_interval == 0 || tempo_interval_samples == 0)
		return notelib_answer_failure_unknown;
	struct notelib_internals* internals = (struct notelib_internals*)notelib_state;
	notelib_track_uint track_count = internals->track_count;
	struct notelib_track* track_ptr;
	for(notelib_track_uint track_index = 0; track_index < track_count; ++track_index){
		track_ptr = notelib_internals_get_track(notelib_state, track_index);
		if(track_ptr->tempo_ceil_interval_samples == 0)
			goto track_found;
	}
	return notelib_answer_failure_unknown;
track_found:;
	track_ptr->initialized_channel_buffer_size = initialized_channel_buffer_size;
	bool initialized_channel_buffer_inline = notelib_track_is_initialized_channel_buffer_internal(internals, track_ptr);
	void* ptr_to_free_at_failure;
	size_t circular_initialized_channel_buffer_size = notelib_internals_sizeof_track_initialized_channel_buffer(initialized_channel_buffer_size);
	void* initialized_channel_buffer_position;
	size_t command_queue_size = internals->command_queue_size;
	if(!initialized_channel_buffer_inline){
		initialized_channel_buffer_position = malloc(circular_initialized_channel_buffer_size);
		if(initialized_channel_buffer_position == NULL)
			return notelib_answer_failure_unknown;
		ptr_to_free_at_failure = initialized_channel_buffer_position;
	}else{
		ptr_to_free_at_failure = NULL;
		initialized_channel_buffer_position = notelib_track_get_inline_initialized_channel_buffer(track_ptr, command_queue_size);
	}
//	printf("Over here now!\ninitialized_...: %p\n", initialized_channel_buffer_position);
	fflush(stdout);
	size_t sizeof_command_queue = notelib_internals_sizeof_track_command_queue(command_queue_size);
	struct circular_buffer* command_queue =
		circular_buffer_construct
		(&track_ptr->command_queue,
		 sizeof_command_queue,
		 sizeof(struct notelib_command));
	if(command_queue == NULL){
		free(ptr_to_free_at_failure);
		return notelib_status_not_ok;
	}
	struct circular_buffer_liberal_reader_unsynchronized* constructed_initiailized_channel_buffer =
		circular_buffer_liberal_reader_unsynchronized_construct
		(initialized_channel_buffer_position,
		 circular_initialized_channel_buffer_size,
		 initialized_channel_buffer_size);
	if(constructed_initiailized_channel_buffer == NULL){
		free(ptr_to_free_at_failure);
		return notelib_status_not_ok;
	}
	track_ptr->position_sample_offset = 0;
	track_ptr->position = 0;
	track_ptr->tempo_ceil_interval = tempo_interval;
	track_ptr->tempo_ceil_interval_samples = tempo_interval_samples;
	if(!initialized_channel_buffer_inline)
		*notelib_track_get_external_initialized_channel_buffer_ptr(track_ptr, command_queue_size) = initialized_channel_buffer_position;

	return notelib_answer_success;
}
enum notelib_status notelib_reset_track_position(notelib_state_handle notelib_state, notelib_track_uint track_index, notelib_position position){
	struct notelib_command reset_command;
	reset_command.type = notelib_command_type_reset;
	reset_command.position = position;
	if(circular_buffer_write(&notelib_internals_get_track(notelib_state, track_index)->command_queue, &reset_command))
		return notelib_answer_success;
	else
		return notelib_answer_failure_unknown;
}
enum notelib_status notelib_set_track_tempo
(notelib_state_handle notelib_state,
 notelib_track_uint track_index, notelib_position position,
 notelib_position tempo_interval,
 notelib_sample_uint tempo_interval_samples){
	if(tempo_interval_samples == 0 || tempo_interval == 0)
		return notelib_answer_failure_unknown;
	struct notelib_command set_tempo_command;
	set_tempo_command.type = notelib_command_type_set_tempo;
	set_tempo_command.tempo.position_interval = tempo_interval;
	set_tempo_command.tempo.interval = tempo_interval_samples;
	set_tempo_command.position = position;
	if(circular_buffer_write(&notelib_internals_get_track(notelib_state, track_index)->command_queue, &set_tempo_command))
		return notelib_answer_success;
	else
		return notelib_answer_failure_unknown;
}
enum notelib_status notelib_set_track_initialized_channel_buffer_size
(notelib_state_handle notelib_state,
 notelib_track_uint track_index,
 uint32_t initialized_channel_buffer_size){
	struct notelib_track* track_ptr = notelib_internals_get_track(notelib_state, track_index);
	void* ptr_to_free;
	uint16_t command_queue_size = ((struct notelib_internals*)notelib_state)->command_queue_size;
	if(notelib_track_is_initialized_channel_buffer_internal(notelib_state, track_ptr))
		ptr_to_free = NULL;
	else
		ptr_to_free = *notelib_track_get_external_initialized_channel_buffer_ptr(track_ptr, command_queue_size);
	uint32_t initialized_channel_buffer_size_before = track_ptr->initialized_channel_buffer_size;
	track_ptr->initialized_channel_buffer_size = initialized_channel_buffer_size;
	if(notelib_track_is_initialized_channel_buffer_internal(notelib_state, track_ptr)){
		size_t circular_initialized_channel_buffer_size = notelib_internals_sizeof_track_initialized_channel_buffer(initialized_channel_buffer_size);
		void* allocated_initialized_channel_buffer = malloc(circular_initialized_channel_buffer_size);
		if(allocated_initialized_channel_buffer == NULL){
			track_ptr->initialized_channel_buffer_size = initialized_channel_buffer_size_before;
			return notelib_answer_failure_unknown;
		}
		struct circular_buffer_liberal_reader_unsynchronized* constructed_initialized_channel_buffer =
			circular_buffer_liberal_reader_unsynchronized_construct(allocated_initialized_channel_buffer, circular_initialized_channel_buffer_size, initialized_channel_buffer_size);
		if(constructed_initialized_channel_buffer == NULL){
			free(allocated_initialized_channel_buffer);
			track_ptr->initialized_channel_buffer_size = initialized_channel_buffer_size_before;
			return notelib_status_not_ok;
		}
		*notelib_track_get_external_initialized_channel_buffer_ptr(track_ptr, command_queue_size) = constructed_initialized_channel_buffer;
	}
	free(ptr_to_free);
	return notelib_answer_success;
}
enum notelib_status notelib_stop_track(notelib_state_handle notelib_state, notelib_track_uint track_index){
	struct notelib_track* track_ptr = notelib_internals_get_track(notelib_state, track_index);
	if(track_ptr->tempo_ceil_interval_samples != 0){
		track_ptr->tempo_ceil_interval_samples = 0;
		if(!notelib_track_is_initialized_channel_buffer_internal(notelib_state, track_ptr))
			free(*notelib_track_get_external_initialized_channel_buffer_ptr(track_ptr, ((struct notelib_internals*)notelib_state)->command_queue_size));
	}
	return notelib_answer_success;
}

enum notelib_status notelib_play
(notelib_state_handle notelib_state,
 notelib_instrument_uint instrument_index,
 void* trigger_data,
 notelib_track_uint track_index, notelib_position position){
	struct notelib_track* track_ptr = notelib_internals_get_track(notelib_state, track_index);
	if(track_ptr->tempo_ceil_interval_samples == 0)
		return notelib_answer_failure_invalid_track;
	struct circular_buffer_liberal_reader_unsynchronized* initialized_channel_buffer = notelib_track_get_initialized_channel_buffer_ptr(notelib_state, track_ptr);
	struct notelib_instrument* instrument_ptr = notelib_internals_get_instrument(notelib_state, instrument_index);
	notelib_step_uint step_count = instrument_ptr->step_count;
	if(step_count == 0)
		return notelib_answer_failure_invalid_instrument;

	notelib_instrument_state_uint channel_state_size = instrument_ptr->channel_state_size;
	void* channel_data_pre_write_buffer = malloc(channel_state_size);
	if(channel_data_pre_write_buffer == NULL)
		return notelib_answer_failure_bad_alloc;
	struct notelib_processing_step_entry* steps_ptr = notelib_instrument_get_processing_steps(notelib_state, instrument_ptr);
	for(notelib_step_uint i = 0; i < step_count; ++i){
		struct notelib_processing_step_entry* step = steps_ptr + i;
		notelib_processing_step_setup_function setup = step->setup;
		if(setup != NULL)
			setup
			(NOTELIB_INTERNAL_OFFSET_AND_CAST(trigger_data,                  step->trigger_data_offset, void*),
			 NOTELIB_INTERNAL_OFFSET_AND_CAST(channel_data_pre_write_buffer, step->data_offset,         void*));
	}
	if(!circular_buffer_liberal_reader_unsynchronized_write(initialized_channel_buffer, channel_data_pre_write_buffer, channel_state_size)){
		free(channel_data_pre_write_buffer);
		return notelib_answer_failure_insufficient_initialized_channel_buffer_space;
	}

	struct notelib_command play_note_command;
	play_note_command.type = notelib_command_type_note;
	play_note_command.note.instrument_index = instrument_index;
	play_note_command.position = position;
	if(!circular_buffer_write(&track_ptr->command_queue, &play_note_command)){
		for(notelib_step_uint i = 0; i < step_count; ++i){
			struct notelib_processing_step_entry* step = steps_ptr + i;
			notelib_processing_step_cleanup_function cleanup = step->cleanup;
			if(cleanup != NULL)
				cleanup(NOTELIB_INTERNAL_OFFSET_AND_CAST(channel_data_pre_write_buffer, step->data_offset, void*));
		}
		if(!circular_buffer_liberal_reader_unsynchronized_unwrite(initialized_channel_buffer, channel_state_size))
			return notelib_status_not_ok;
		free(channel_data_pre_write_buffer);
		return notelib_answer_failure_insufficient_command_queue_entries;
	}

	free(channel_data_pre_write_buffer);
	return notelib_answer_success;
}

#endif//#ifndef NOTELIB_INTERNAL_H_
