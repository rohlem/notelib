#include "internals.h"

#include "../internal.h"
#include "alignment_utilities.h"
#include "circular_buffer.h"
#include "command.h"
#include "instrument.h"
#include "processing_step_entry.h"
#include "track.h"

#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../util/eclipse_codan_fix.hpp"

notelib_note_id_uint notelib_internals_get_next_note_id(struct notelib_internals* internals){
	notelib_note_id_uint* next_note_id_ptr = &(internals->next_note_id);
	notelib_note_id_uint note_id = *next_note_id_ptr;
	if(note_id == notelib_internals_non_unique_note_id)
		note_id = 0;
	*next_note_id_ptr = note_id + 1;
	return note_id;
}

size_t notelib_internals_offsetof_dual_audio_buffer(notelib_instrument_uint instrument_count, uint16_t instrument_size)
	{return
		 NOTELIB_INTERNAL_ALIGN_TO_NEXT_ALIGNOF
		 (notelib_internals_offsetof_instruments + instrument_count * instrument_size, notelib_sample);}

notelib_sample* notelib_internals_get_audio_buffer(struct notelib_internals* internals)
	{return
		 NOTELIB_INTERNAL_OFFSET_AND_CAST
		 (internals,
		  notelib_internals_offsetof_dual_audio_buffer(internals->instrument_count, internals->instrument_size),
		  notelib_sample*);}

size_t notelib_internals_sizeof_dual_audio_buffer(uint16_t dual_buffer_size)
	{return (1 + NOTELIB_INTERNAL_USE_INTERMEDIATE_MIXING_BUFFER)*dual_buffer_size*sizeof(notelib_sample);}

size_t notelib_internals_offsetof_tracks
(notelib_instrument_uint instrument_count, uint16_t instrument_size, uint16_t dual_buffer_size)
	{return
		 NOTELIB_INTERNAL_ALIGN_TO_NEXT_ALIGNOF
		 (notelib_internals_offsetof_dual_audio_buffer(instrument_count, instrument_size)
		  + notelib_internals_sizeof_dual_audio_buffer(dual_buffer_size),
		  struct notelib_track);}
#ifndef NOTELIB_NO_IMMEDIATE_TRACK
size_t notelib_internals_offsetof_track_immediate
(notelib_instrument_uint instrument_count, uint16_t instrument_size, uint16_t dual_buffer_size)
	{return notelib_internals_offsetof_tracks(instrument_count, instrument_size, dual_buffer_size);}
#endif//#ifndef NOTELIB_NO_IMMEDIATE_TRACK
size_t notelib_internals_offsetof_regular_tracks
(notelib_instrument_uint instrument_count, uint16_t instrument_size, uint16_t dual_buffer_size
#ifndef NOTELIB_NO_IMMEDIATE_TRACK
 , uint16_t queued_immediate_command_count, uint16_t reserved_inline_immediate_initialized_channel_buffer_size
#endif//#ifndef NOTELIB_NO_IMMEDIATE_TRACK
)
#ifndef NOTELIB_NO_IMMEDIATE_TRACK
	{return
		(notelib_internals_offsetof_tracks(instrument_count, instrument_size, dual_buffer_size)
		 + notelib_internals_sizeof_track_immediate(queued_immediate_command_count, reserved_inline_immediate_initialized_channel_buffer_size));}
#else//#ifndef NOTELIB_NO_IMMEDIATE_TRACK
	{return notelib_internals_offsetof_tracks(instrument_count, instrument_size, dual_buffer_size);}
#endif

#ifndef NOTELIB_NO_IMMEDIATE_TRACK
size_t notelib_internals_offsetof_track_immediate_in_tracks() {return 0;}
#endif//#ifndef NOTELIB_NO_IMMEDIATE_TRACK
size_t notelib_internals_offsetof_regular_tracks_in_tracks
(
#ifndef NOTELIB_NO_IMMEDIATE_TRACK
 uint16_t queued_immediate_command_count, uint16_t reserved_inline_immediate_initialized_channel_buffer_size
#endif//#ifndef NOTELIB_NO_IMMEDIATE_TRACK
){
#ifndef NOTELIB_NO_IMMEDIATE_TRACK
	return notelib_internals_sizeof_track_immediate(queued_immediate_command_count, reserved_inline_immediate_initialized_channel_buffer_size);
#else//#ifndef NOTELIB_NO_IMMEDIATE_TRACK
	return 0;
#endif//#ifndef NOTELIB_NO_IMMEDIATE_TRACK
}

void* notelib_internals_get_tracks(struct notelib_internals* internals)
	{return
		 NOTELIB_INTERNAL_OFFSET_AND_CAST
		 (internals,
		  notelib_internals_offsetof_tracks(internals->instrument_count, internals->instrument_size, internals->dual_buffer_size),
		  void*);}
struct notelib_track* notelib_internals_get_regular_tracks(struct notelib_internals* internals){
	const size_t offsetof_regular_tracks_in_tracks =
		notelib_internals_offsetof_regular_tracks_in_tracks(
#ifndef NOTELIB_NO_IMMEDIATE_TRACK
		  internals->immediate_command_queue_size, internals->reserved_inline_immediate_initialized_channel_buffer_size
#endif//#ifndef NOTELIB_NO_IMMEDIATE_TRACK
		);
	return
		NOTELIB_INTERNAL_OFFSET_AND_CAST
		(notelib_internals_get_tracks(internals),
		 offsetof_regular_tracks_in_tracks,
		 struct notelib_track*);
}

bool notelib_track_is_initialized_channel_buffer_internal(const struct notelib_internals* internals, const struct notelib_track* track_ptr)
	{return track_ptr->data.initialized_channel_buffer_size <= internals->reserved_inline_initialized_channel_buffer_size;}
#ifndef NOTELIB_NO_IMMEDIATE_TRACK
bool notelib_track_immediate_is_initialized_channel_buffer_internal(const struct notelib_internals* internals, const struct notelib_track_immediate* track_ptr)
	{return track_ptr->data.initialized_channel_buffer_size <= internals->reserved_inline_immediate_initialized_channel_buffer_size;}
#endif//#ifndef NOTELIB_NO_IMMEDIATE_TRACK

size_t notelib_internals_sizeof_track_command_queue(uint16_t queued_command_count)
	{return circular_buffer_sizeof(sizeof(struct notelib_command), queued_command_count);}
size_t notelib_internals_sizeof_track_immediate_command_queue(uint16_t queued_command_count)
	{return circular_buffer_sizeof(sizeof(struct notelib_command_immediate), queued_command_count);}

size_t notelib_internals_sizeof_track_initialized_channel_buffer(uint16_t initialized_channel_buffer_size)
	{return circular_buffer_liberal_reader_unsynchronized_sizeof(initialized_channel_buffer_size);}

size_t notelib_internals_sizeof_track(uint16_t queued_command_count, uint16_t reserved_inline_initialized_channel_buffer_size)
	{return
		 NOTELIB_INTERNAL_ALIGN_TO_NEXT_ALIGNOF
		 (NOTELIB_INTERNAL_ALIGN_TO_NEXT_ALIGNOF
		  (notelib_track_offsetof_command_queue + notelib_internals_sizeof_track_command_queue(queued_command_count),
		   struct circular_buffer_liberal_reader_unsynchronized)
		  + notelib_internals_sizeof_track_initialized_channel_buffer(reserved_inline_initialized_channel_buffer_size),
		  struct notelib_track);}
#ifndef NOTELIB_NO_IMMEDIATE_TRACK
size_t notelib_internals_sizeof_track_immediate(uint16_t queued_immediate_command_count, uint16_t reserved_inline_immediate_initialized_channel_buffer_size)
	{return
		 NOTELIB_INTERNAL_ALIGN_TO_NEXT_ALIGNOF
		 (NOTELIB_INTERNAL_ALIGN_TO_NEXT_ALIGNOF
		  (notelib_track_immediate_offsetof_command_queue + notelib_internals_sizeof_track_immediate_command_queue(queued_immediate_command_count),
		   struct circular_buffer_liberal_reader_unsynchronized)
		  + notelib_internals_sizeof_track_initialized_channel_buffer(reserved_inline_immediate_initialized_channel_buffer_size),
		  struct notelib_track);}
#endif//#ifndef NOTELIB_NO_IMMEDIATE_TRACK

#ifndef NOTELIB_NO_IMMEDIATE_TRACK
struct notelib_track_immediate* notelib_internals_get_track_immediate(struct notelib_internals* internals)
	{return
		 NOTELIB_INTERNAL_OFFSET_AND_CAST
		 (notelib_internals_get_tracks(internals),
		  notelib_internals_offsetof_track_immediate_in_tracks(),
		  struct notelib_track_immediate*);}
#endif//#ifndef NOTELIB_NO_IMMEDIATE_TRACK
struct notelib_track* notelib_internals_get_regular_track(struct notelib_internals* internals, notelib_track_uint track_index)
	{return
		 NOTELIB_INTERNAL_OFFSET_AND_CAST
		 (notelib_internals_get_regular_tracks(internals),
		  track_index * notelib_internals_sizeof_track(internals->command_queue_size, internals->reserved_inline_initialized_channel_buffer_size),
		  struct notelib_track*);}

size_t notelib_internals_size_requirements(const struct notelib_params* params)
	{return
		 notelib_internals_offsetof_regular_tracks
		 (params->instrument_count,
		  notelib_internals_sizeof_instrument(params->inline_step_count, params->reserved_inline_state_space),
		  params->internal_dual_buffer_size
#ifndef NOTELIB_NO_IMMEDIATE_TRACK
		  , params->queued_immediate_command_count+1, //adding 1 because of the design flaw that one buffer element is always introduced
		  params->reserved_inline_immediate_initialized_channel_buffer_size + (params->reserved_inline_immediate_initialized_channel_buffer_size>0) //adding 1 because of the design flaw that one buffer element is always introduced
#endif//#ifndef NOTELIB_NO_IMMEDIATE_TRACK
		 )
		 + params->regular_track_count
		   * notelib_internals_sizeof_track
		     (params->queued_command_count+1, //adding 1 because of the design flaw that one buffer element is always introduced
		      params->reserved_inline_initialized_channel_buffer_size + (params->reserved_inline_initialized_channel_buffer_size>0)); /*adding 1 because of the design flaw that one buffer element is always introduced*/}

enum notelib_status notelib_internals_init(void* position, size_t space_available, const struct notelib_params* params){
	// initial size check
	if(space_available < notelib_internals_size_requirements(params))
		return notelib_answer_failure_unknown;
#ifdef NOTELIB_NO_IMMEDIATE_TRACK
	if(params->regular_track_count == 0)
		return notelib_answer_failure_no_tracks;
#endif//#ifdef NOTELIB_NO_IMMEDIATE_TRACK

	// initialization of internals' members
	struct notelib_internals* internals = position;
	const notelib_instrument_uint instrument_count = params->instrument_count;
	internals->instrument_count  = instrument_count;
	internals->inline_step_count = params->inline_step_count;
	internals->reserved_inline_state_space = params->reserved_inline_state_space;
	const uint16_t dual_buffer_size = params->internal_dual_buffer_size;
	internals->dual_buffer_size = dual_buffer_size;
	const notelib_track_uint track_count = params->regular_track_count;
	internals->track_count = track_count;
#ifndef NOTELIB_NO_IMMEDIATE_TRACK
	const uint16_t queued_immediate_command_count = params->queued_immediate_command_count+1; //adding 1 because of the design flaw that one buffer element is always introduced
	internals->immediate_command_queue_size = queued_immediate_command_count;
	uint16_t reserved_inline_immediate_initialized_channel_buffer_size = params->reserved_inline_immediate_initialized_channel_buffer_size;
	reserved_inline_immediate_initialized_channel_buffer_size += reserved_inline_immediate_initialized_channel_buffer_size > 0; //adding 1 because of the design flaw that one buffer element is always introduced
	internals->reserved_inline_immediate_initialized_channel_buffer_size = reserved_inline_immediate_initialized_channel_buffer_size;
#endif//#ifndef NOTELIB_NO_IMMEDIATE_TRACK
	const uint16_t queued_command_count = params->queued_command_count+1; //adding 1 because of the design flaw that one buffer element is always introduced
	internals->command_queue_size = queued_command_count;
	uint16_t reserved_inline_initialized_channel_buffer_size = params->reserved_inline_initialized_channel_buffer_size;
	reserved_inline_initialized_channel_buffer_size += reserved_inline_initialized_channel_buffer_size > 0; //adding 1 because of the design flaw that one buffer element is always introduced
	internals->reserved_inline_initialized_channel_buffer_size = reserved_inline_initialized_channel_buffer_size;
	const size_t instrument_size = notelib_internals_sizeof_instrument(params->inline_step_count, params->reserved_inline_state_space);
	internals->instrument_size = instrument_size;

	// sanity (size) checks
#ifndef NOTELIB_NO_IMMEDIATE_TRACK
	size_t sizeof_track_immediate = notelib_internals_sizeof_track_immediate(queued_immediate_command_count, reserved_inline_immediate_initialized_channel_buffer_size);
	const size_t offsetof_track_immediate = notelib_internals_offsetof_track_immediate(instrument_count, instrument_size, dual_buffer_size);
#endif//#ifndef NOTELIB_NO_IMMEDIATE_TRACK
	const size_t offsetof_regular_tracks = notelib_internals_offsetof_regular_tracks(instrument_count, instrument_size, dual_buffer_size
#ifndef NOTELIB_NO_IMMEDIATE_TRACK
		 , queued_immediate_command_count, reserved_inline_immediate_initialized_channel_buffer_size
#endif//#ifndef NOTELIB_NO_IMMEDIATE_TRACK
);
#ifndef NOTELIB_NO_IMMEDIATE_TRACK
	if(sizeof_track_immediate > offsetof_regular_tracks - offsetof_track_immediate)
		return notelib_answer_failure_unknown;
	if(track_count > 0){ //if NOTELIB_NO_IMMEDIATE_TRACK we already checked for 0 before
#endif//#ifndef NOTELIB_NO_IMMEDIATE_TRACK
		const size_t sizeof_track = notelib_internals_sizeof_track(queued_command_count, reserved_inline_initialized_channel_buffer_size);
		if(sizeof_track > (space_available - offsetof_regular_tracks)/track_count)
			return notelib_answer_failure_unknown;
#ifndef NOTELIB_NO_IMMEDIATE_TRACK
	}

	// "object" initialization
	enum notelib_status immediate_track_data_setup_status =
		notelib_track_immediate_data_setup
		(internals, notelib_internals_get_track_immediate(internals),
		 params->reserved_inline_immediate_initialized_channel_buffer_size);
	if(immediate_track_data_setup_status != notelib_answer_success)
		return immediate_track_data_setup_status;
#endif//#ifndef NOTELIB_NO_IMMEDIATE_TRACK
	for(notelib_instrument_uint instrument_index = 0; instrument_index < instrument_count; ++instrument_index)
		notelib_internals_get_instrument(internals, instrument_index)->step_count = 0;
	for(notelib_track_uint track_index = 0; track_index < track_count; ++track_index)
		notelib_track_disable(notelib_internals_get_regular_track(internals, track_index));

	return notelib_answer_success;
}

enum notelib_status notelib_internals_deinit(notelib_state_handle state_handle){
	struct notelib_internals* internals = (struct notelib_internals*)state_handle;
#ifndef NOTELIB_NO_IMMEDIATE_TRACK
	struct notelib_track_immediate* track_immediate_ptr = notelib_internals_get_track_immediate(internals);
	if(!notelib_track_immediate_is_initialized_channel_buffer_internal(internals, track_immediate_ptr))
		free(notelib_track_immediate_get_external_initialized_channel_buffer_ptr(track_immediate_ptr, internals->immediate_command_queue_size));
#endif//#ifndef NOTELIB_NO_IMMEDIATE_TRACK
	notelib_track_uint track_count = internals->track_count;
	for(notelib_track_uint track_index = 0; track_index < track_count; ++track_index){
		struct notelib_track* track_ptr = notelib_internals_get_regular_track(internals, track_index);
		if(notelib_track_is_disabled(track_ptr))
			continue;
		if(!notelib_track_is_initialized_channel_buffer_internal(internals, track_ptr))
			free(notelib_track_get_external_initialized_channel_buffer_ptr(track_ptr, internals->command_queue_size));
		notelib_track_disable(track_ptr);
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
			void* current_channel_data = NOTELIB_INTERNAL_OFFSET_AND_CAST(channel_data, channel_index*NOTELIB_CHANNEL_SIZEOF_SINGLE(instrument->channel_data_size), void*);
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
(struct notelib_channel* channel_state_front, struct notelib_channel* channel_state_back, size_t channel_state_size,
 notelib_step_uint step_count, struct notelib_processing_step_entry* steps,
 notelib_sample* instrument_mix_buffer, notelib_sample* channel_mix_buffer, notelib_sample_uint samples_requested,
 notelib_channel_uint* active_channel_count_ptr){
	// general setup
	struct notelib_processing_step_entry* steps_end = steps + step_count; //end iterator for steps array
	bool at_back = false; //whether the current channel is at the back, moving backwards, looking for active channels to replace the dead front with
	void* current_channel_state_data; //the channel that is currently being queried
	struct notelib_channel* previous_channel_state_back; //cache for unifying branches

	// this once was a do-while-true with custom break points, but with two entry points it's not really worth it
	loop_start_at_front: //initialization reused as assignment in loop entry
		current_channel_state_data = channel_state_front->data;
	loop_start_general:;
		// first generate the samples by calling every step's step function
		struct notelib_processing_step_entry* current_step = steps;
		notelib_sample_uint produced_samples = samples_requested;
		notelib_sample* filled_channel_mix_buffer = NULL; //the first step receives NULL as input buffer
		do{
			notelib_sample_uint newly_produced_samples =
				current_step->step
				(filled_channel_mix_buffer, channel_mix_buffer,
				 produced_samples, NOTELIB_INTERNAL_OFFSET_AND_CAST(current_channel_state_data, current_step->data_offset, void*));
			produced_samples = MIN(produced_samples, newly_produced_samples);
			++current_step;
		if(current_step == steps_end) break;
			filled_channel_mix_buffer = channel_mix_buffer; //subsequent steps receive the channel_mix_buffer as input buffer
		}while(true);

		// add the generated samples to the general mix buffer
		for(notelib_sample_uint sample_index = 0; sample_index < produced_samples; ++sample_index)
			instrument_mix_buffer[sample_index] += channel_mix_buffer[sample_index];

		// if moving backwards
		if(at_back){
			previous_channel_state_back = channel_state_back; //make of the address to copy the instrument from, in case it is still alive
			channel_state_back  = NOTELIB_INTERNAL_OFFSET_AND_CAST(channel_state_back,  -channel_state_size, struct notelib_channel*); //move the back iterator
		}
		// distinguish and handle in-/active channels
		if(produced_samples < samples_requested){ //now inactive
			--*active_channel_count_ptr; //decrement number of active channels
			// call every step's cleanup function, if the respective pointer is valid
			current_step = steps;
			do{
				notelib_processing_step_cleanup_function cleanup = current_step->cleanup;
				if(cleanup != NULL)
					cleanup(NOTELIB_INTERNAL_OFFSET_AND_CAST(current_channel_state_data, current_step->data_offset, void*));
				++current_step;
			}while(current_step != steps_end);
	if(channel_state_front == channel_state_back) return; //end if this was the last channel to handle, f.e. because we just reached the (already inactive) front again
			// move to the back, because the front is now inactive
			current_channel_state_data = channel_state_back->data;
			at_back = true;
	goto loop_start_general; //continue with the next state (*current_channel_state)
		}else{ //still active
			if(at_back){ //if the front is inactive and we were looking for an still active replacement
				memcpy(channel_state_front, previous_channel_state_back, channel_state_size); //replace by memcpy
				at_back = false; //note that we now switch back to the front, so we don't care about the back anymore
			}
	if(channel_state_front == channel_state_back) return; //end if all active channels have been handled
			channel_state_front = NOTELIB_INTERNAL_OFFSET_AND_CAST(channel_state_front,  channel_state_size, struct notelib_channel*); //move the front iterator
	goto loop_start_at_front; //continue with the front
		}
	//unreachable
}

void notelib_internals_fill_buffer_part(struct notelib_internals* internals, notelib_sample* out, notelib_sample_uint samples_requested, notelib_channel_uint* still_active_channel_count){
	notelib_instrument_uint instrument_count = internals->instrument_count;
#if NOTELIB_INTERNAL_USE_INTERMEDIATE_MIXING_BUFFER
	notelib_sample* instrument_mix_buffer = notelib_internals_get_audio_buffer(internals);
#else//#if NOTELIB_INTERNAL_USE_INTERMEDIATE_MIXING_BUFFER
	notelib_sample* instrument_mix_buffer = out;
#endif//#if NOTELIB_INTERNAL_USE_INTERMEDIATE_MIXING_BUFFER
#if NOTELIB_INTERNAL_USE_EXPLICIT_MEMSET
	memset(instrument_mix_buffer, 0, sizeof(notelib_sample)*samples_requested);
#else//#if NOTELIB_INTERNAL_USE_EXPLICIT_MEMSET
	for(uint16_t i = 0; i < samples_requested; ++i)
		instrument_mix_buffer[i] = 0;
#endif//#if NOTELIB_INTERNAL_USE_EXPLICIT_MEMSET
#if NOTELIB_INTERNAL_USE_INTERMEDIATE_MIXING_BUFFER
	notelib_sample* channel_mix_buffer = instrument_mix_buffer + internals->dual_buffer_size;
#else//#if NOTELIB_INTERNAL_USE_INTERMEDIATE_MIXING_BUFFER
	notelib_sample* channel_mix_buffer = notelib_internals_get_audio_buffer(internals);
#endif//#if NOTELIB_INTERNAL_USE_INTERMEDIATE_MIXING_BUFFER

	//TODO: optimization opportunity (probably behind preprocessor/config flag): link notes with tracks (presumably in struct notelib_channel) so we can safely generate bigger chunks of samples from less active tracks

	notelib_sample_uint samples_generated = 0;
	do{//samples_requested cannot be 0
	#ifndef NOTELIB_NO_IMMEDIATE_TRACK
		//EXECUTE COMMANDS TRACK IMMEDIATE
		struct notelib_track_immediate* immediate_track_ptr = notelib_internals_get_track_immediate(internals);
		struct circular_buffer* immediate_command_queue_ptr = notelib_track_immediate_get_command_queue(immediate_track_ptr);
		const struct notelib_command_immediate* immediate_command_ptr;
		do{
			immediate_command_ptr = circular_buffer_direct_read_commence(immediate_command_queue_ptr);
		if(immediate_command_ptr == NULL) break;
			const union notelib_command_data* command_data_ptr = &(immediate_command_ptr->data);
			switch(immediate_command_ptr->type){
			case notelib_command_type_note:;
				const struct notelib_command_note* note_command = &(command_data_ptr->note);
				notelib_instrument_uint instrument_index = note_command->instrument_index;
				struct notelib_instrument* instrument = notelib_internals_get_instrument(internals, instrument_index);
				if(instrument->step_count > 0){
					struct circular_buffer_liberal_reader_unsynchronized* initialized_channel_state_buffer =
						notelib_track_immediate_get_initialized_channel_buffer_ptr(internals, immediate_track_ptr);
					notelib_instrument_state_uint channel_data_size = instrument->channel_data_size;
					size_t channel_state_size = NOTELIB_CHANNEL_SIZEOF_SINGLE(channel_data_size);
					struct notelib_channel* instrument_state_data = notelib_instrument_get_state_data(internals, instrument);
					notelib_channel_uint* specific_still_active_channel_count = still_active_channel_count + instrument_index;
					notelib_channel_uint last_channel_index = *specific_still_active_channel_count;
					++*specific_still_active_channel_count;
					struct notelib_channel* channel_state_ptr =
						NOTELIB_INTERNAL_OFFSET_AND_CAST
						(instrument_state_data,
						 last_channel_index*channel_state_size,
						 struct notelib_channel*);
					channel_state_ptr->current_note_id = note_command->note_id;
					circular_buffer_liberal_reader_unsynchronized_read
					(initialized_channel_state_buffer,
					 channel_data_size,
					 channel_state_ptr->data);
				}else
					fputs("WARNING (notelib): Received (immediate) note command referencing disabled instrument! (Ignoring...)\n", stderr);
				break;
			case notelib_command_type_reset:
				fputs("WARNING (notelib): Received (position) reset command on immediate track! (Ignoring...)\n", stderr);
				break;
			case notelib_command_type_set_tempo:;
				fputs("WARNING (notelib): Received set_tempo command on immediate track! (Ignoring...)\n", stderr);
				break;
			case notelib_command_type_trigger:;
				const struct notelib_command_trigger* command_trigger = &command_data_ptr->trigger;
				command_trigger->trigger_function(command_trigger->userdata);
				break;
			case notelib_command_type_alter:;
				const struct notelib_command_alter* command_alter = &command_data_ptr->alter;
				for(notelib_instrument_uint i = 0; i < instrument_count; ++i){
					struct notelib_instrument* instrument = notelib_internals_get_instrument(internals, i);
					notelib_step_uint step_count = instrument->step_count;
					if(step_count > 0){
						notelib_channel_uint active_channel_count = still_active_channel_count[instrument_index];
						struct notelib_channel* instrument_state_data = notelib_instrument_get_state_data(internals, instrument);
						size_t channel_state_size = NOTELIB_CHANNEL_SIZEOF_SINGLE(instrument->channel_data_size);
						for(notelib_channel_uint j = 0; j < active_channel_count; ++j){
							struct notelib_channel* channel_state_ptr =
								NOTELIB_INTERNAL_OFFSET_AND_CAST
								(instrument_state_data,
								 j*channel_state_size,
								 struct notelib_channel*);
							if(channel_state_ptr->current_note_id == command_alter->note_id){
								command_alter->alter_function(channel_state_ptr, command_alter->userdata);
				goto immediate_alter_finished;
							}
						}
					}
				}
				//did not find the referenced note... is this even an error?
				immediate_alter_finished:
					break;
			}
			circular_buffer_direct_read_commit(immediate_command_queue_ptr);
		}while(true);
	#endif//#ifndef NOTELIB_NO_IMMEDIATE_TRACK

		//CHECK HOW MANY SAMPLES TO GENERATE BEFORE COMMANDS
		notelib_sample_uint samples_until_next_position = samples_requested - samples_generated;
		notelib_track_uint track_count = internals->track_count;
		for(notelib_track_uint track_index = 0; track_index < track_count; ++track_index){
			struct notelib_track* track_ptr = notelib_internals_get_regular_track(internals, track_index);
			if(notelib_track_is_disabled(track_ptr))
				continue;
			notelib_position old_position = track_ptr->position;
			notelib_sample_uint samples_until_next_position_on_track = notelib_track_get_tempo_interval(track_ptr, old_position, old_position+1) - track_ptr->position_sample_offset;
			samples_until_next_position = MIN(samples_until_next_position, samples_until_next_position_on_track);
		}

		//GENERATE SAMPLES
		for(notelib_instrument_uint instrument_index = 0; instrument_index < instrument_count; ++instrument_index){
			struct notelib_instrument* instrument_ptr = notelib_internals_get_instrument(internals, instrument_index);
			notelib_step_uint step_count = instrument_ptr->step_count;
			if(step_count > 0){
				notelib_channel_uint active_channel_count = still_active_channel_count[instrument_index];
				if(active_channel_count > 0){
					struct notelib_processing_step_entry* steps = notelib_instrument_get_processing_steps(internals, instrument_ptr);
					size_t channel_state_size = NOTELIB_CHANNEL_SIZEOF_SINGLE(instrument_ptr->channel_data_size);
					struct notelib_channel* channel_state_front = notelib_instrument_get_state_data(internals, instrument_ptr);
					struct notelib_channel* channel_state_back  = NOTELIB_INTERNAL_OFFSET_AND_CAST(channel_state_front, (active_channel_count - 1) * channel_state_size, void*);
					notelib_internals_execute_instrument_steps
					(channel_state_front, channel_state_back, channel_state_size,
					 step_count, steps,
					 instrument_mix_buffer + samples_generated, channel_mix_buffer, samples_until_next_position,
					 still_active_channel_count + instrument_index);
				}
			}
		}

		//EXECUTE COMMANDS
		for(notelib_track_uint track_index = 0; track_index < track_count; ++track_index){
			struct notelib_track* track_ptr = notelib_internals_get_regular_track(internals, track_index);
			if(notelib_track_is_disabled(track_ptr))
				continue;
			notelib_position old_position = track_ptr->position;
			notelib_position new_position = old_position + 1;
			notelib_sample_uint new_sample_offset = track_ptr->position_sample_offset + samples_until_next_position;
			notelib_sample_uint tempo_distance_sample_count = notelib_track_get_tempo_interval(track_ptr, old_position, new_position);
			if(new_sample_offset < tempo_distance_sample_count){
				track_ptr->position_sample_offset = new_sample_offset;
				continue;
			}
			if(new_sample_offset > tempo_distance_sample_count) //every command execution is (or should be) at the transition into the new position
				fputs("ASSERTION FAILED: new_sample_offset > tempo_distance_sample_count!\n", stderr);
			struct circular_buffer* command_queue_ptr = notelib_track_get_command_queue(track_ptr);
			const struct notelib_command* command_ptr;
			do{
				command_ptr = circular_buffer_direct_read_commence(command_queue_ptr);
			if(command_ptr == NULL) break;
				notelib_position command_position = command_ptr->position;
			if(command_position > new_position) break;
				const union notelib_command_data* command_data_ptr = &(command_ptr->data);
				switch(command_ptr->type){
				case notelib_command_type_note:;
					const struct notelib_command_note* note_command = &(command_data_ptr->note);
					notelib_instrument_uint instrument_index = note_command->instrument_index;
					struct notelib_instrument* instrument = notelib_internals_get_instrument(internals, instrument_index);
					if(instrument->step_count > 0){
						struct circular_buffer_liberal_reader_unsynchronized* initialized_channel_state_buffer =
							notelib_track_get_initialized_channel_buffer_ptr(internals, track_ptr);
						notelib_instrument_state_uint channel_data_size = instrument->channel_data_size;
						size_t channel_state_size = NOTELIB_CHANNEL_SIZEOF_SINGLE(channel_data_size);
						struct notelib_channel* instrument_state_data = notelib_instrument_get_state_data(internals, instrument);
						notelib_channel_uint* specific_still_active_channel_count = still_active_channel_count + instrument_index;
						notelib_channel_uint last_channel_index = *specific_still_active_channel_count;
						++*specific_still_active_channel_count;
						struct notelib_channel* channel_state_ptr =
							NOTELIB_INTERNAL_OFFSET_AND_CAST
							(instrument_state_data,
							 last_channel_index*channel_state_size,
							 struct notelib_channel*);
						channel_state_ptr->current_note_id = note_command->note_id;
						circular_buffer_liberal_reader_unsynchronized_read
						(initialized_channel_state_buffer,
						 channel_data_size,
						 channel_state_ptr->data);
					}else
						fputs("WARNING (notelib): Received note command referencing disabled instrument! (Ignoring...)\n", stderr);
					break;
				case notelib_command_type_reset:
					new_position = 0;
					break;
				case notelib_command_type_set_tempo:;
					const struct notelib_tempo* command_tempo = &command_data_ptr->tempo;
					notelib_position command_tempo_ceil_interval = command_tempo->position_interval;
					notelib_sample_uint command_tempo_interval_samples = command_tempo->interval;
					//TODO: to consider as future extension: command_tempo->interval == 0 could signal unchanged interval_samples (to only scale the position base tempo) - see also notelib_start_track
					if(command_tempo_ceil_interval != 0 && command_tempo_interval_samples != 0){
						track_ptr->tempo_ceil_interval = command_tempo_ceil_interval;
						track_ptr->tempo_ceil_interval_samples = command_tempo_interval_samples;
					}else
						fputs("WARNING (notelib): Received halting set tempo command! (Ignoring...)\n", stderr);
					break;
				case notelib_command_type_trigger:;
					const struct notelib_command_trigger* command_trigger = &command_data_ptr->trigger;
					command_trigger->trigger_function(command_trigger->userdata);
					break;
				case notelib_command_type_alter:;
					const struct notelib_command_alter* command_alter = &command_data_ptr->alter;
					for(notelib_instrument_uint i = 0; i < instrument_count; ++i){
						struct notelib_instrument* instrument = notelib_internals_get_instrument(internals, i);
						notelib_step_uint step_count = instrument->step_count;
						if(step_count > 0){
							notelib_channel_uint active_channel_count = still_active_channel_count[instrument_index];
							struct notelib_channel* instrument_state_data = notelib_instrument_get_state_data(internals, instrument);
							size_t channel_state_size = NOTELIB_CHANNEL_SIZEOF_SINGLE(instrument->channel_data_size);
							for(notelib_channel_uint j = 0; j < active_channel_count; ++j){
								struct notelib_channel* channel_state_ptr =
									NOTELIB_INTERNAL_OFFSET_AND_CAST
									(instrument_state_data,
									 j*channel_state_size,
									 struct notelib_channel*);
								if(channel_state_ptr->current_note_id == command_alter->note_id){
									command_alter->alter_function(channel_state_ptr, command_alter->userdata);
					goto alter_finished;
								}
							}
						}
					}
					//did not find the referenced note... is this even an error?
					alter_finished:
						break;
				}
				circular_buffer_direct_read_commit(command_queue_ptr);
			}while(true);
			track_ptr->position_sample_offset = 0;
			track_ptr->position = new_position;
		}

		samples_generated += samples_until_next_position;
	}while(samples_generated < samples_requested);

	//OUTPUT PHASE
#if NOTELIB_INTERNAL_USE_INTERMEDIATE_MIXING_BUFFER
	#if NOTELIB_INTERNAL_USE_EXPLICIT_MEMCPY
	memcpy(out, instrument_mix_buffer, sizeof(notelib_sample)*samples_requested);
	#else//#if NOTELIB_INTERNAL_USE_EXPLICIT_MEMCPY
	for(notelib_sample_uint sample_index = 0; sample_index < samples_requested; ++sample_index)
		out[sample_index] = instrument_mix_buffer[sample_index];
	#endif//#if NOTELIB_INTERNAL_USE_EXPLICIT_MEMCPY
#endif//#if NOTELIB_INTERNAL_USE_INTERMEDIATE_MIXING_BUFFER
}

void notelib_internals_fill_buffer(struct notelib_internals* internals, notelib_sample* out, notelib_sample_uint samples_requested){
	notelib_instrument_uint instrument_count = internals->instrument_count;
	//This VLA is a slight memory concern;
	//being only 255 entries maximum most stacks should be able to handle it (and if you're using that many concurrent instruments you should be able to spare it anyway),
	//but it might still be worth it adding a cache variable to struct notelib_instrument directly.
	notelib_channel_uint still_active_channel_count[instrument_count];
	for(notelib_instrument_uint instrument_index = 0; instrument_index < instrument_count; ++instrument_index){
		struct notelib_instrument* instrument_ptr = notelib_internals_get_instrument(internals, instrument_index);
		if(instrument_ptr->step_count > 0)
			still_active_channel_count[instrument_index] = atomic_load_explicit(&instrument_ptr->active_channel_count, memory_order_relaxed);
	}
	notelib_sample_uint audio_buffer_size = internals->dual_buffer_size;
	if(audio_buffer_size == 0)
		return; //all hope lost, lol
	notelib_sample_uint full_passes     = samples_requested / audio_buffer_size;
	notelib_sample_uint final_pass_size = samples_requested % audio_buffer_size;
	for(notelib_sample_uint pass = 0; pass < full_passes; ++pass){
		notelib_internals_fill_buffer_part(internals, out, audio_buffer_size, still_active_channel_count);
		out += audio_buffer_size;
	}
	if(final_pass_size > 0)
		notelib_internals_fill_buffer_part(internals, out, final_pass_size, still_active_channel_count);
	for(notelib_instrument_uint instrument_index = 0; instrument_index < instrument_count; ++instrument_index)
		atomic_store_explicit(&notelib_internals_get_instrument(internals, instrument_index)->active_channel_count, still_active_channel_count[instrument_index], memory_order_release);
}
