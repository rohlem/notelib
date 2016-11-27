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

size_t notelib_internals_offsetof_tracks(notelib_instrument_uint instrument_count, uint16_t instrument_size, uint16_t dual_buffer_size)
	{return
		 NOTELIB_INTERNAL_ALIGN_TO_NEXT_ALIGNOF
		 (notelib_internals_offsetof_dual_audio_buffer(instrument_count, instrument_size)
		  + notelib_internals_sizeof_dual_audio_buffer(dual_buffer_size),
		  struct circular_buffer);}

struct notelib_track* notelib_internals_get_tracks(struct notelib_internals* internals)
	{return
		 NOTELIB_INTERNAL_OFFSET_AND_CAST
		 (internals,
		  notelib_internals_offsetof_tracks(internals->instrument_count, internals->instrument_size, internals->dual_buffer_size),
		  struct notelib_track*);}

bool notelib_track_is_initialized_channel_buffer_internal(const struct notelib_internals* internals, const struct notelib_track* track_ptr)
	{return track_ptr->initialized_channel_buffer_size <= internals->reserved_inline_initialized_channel_buffer_size;}

size_t notelib_internals_sizeof_track_command_queue(uint16_t queued_command_count)
	{return circular_buffer_sizeof(sizeof(struct notelib_command), queued_command_count);}

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

struct notelib_track* notelib_internals_get_track(struct notelib_internals* internals, notelib_track_uint track_index)
	{return
		 NOTELIB_INTERNAL_OFFSET_AND_CAST
		 (notelib_internals_get_tracks(internals),
		  track_index * notelib_internals_sizeof_track(internals->command_queue_size, internals->reserved_inline_initialized_channel_buffer_size),
		  struct notelib_track*);}

size_t notelib_internals_size_requirements(const struct notelib_params* params)
	{return
		 notelib_internals_offsetof_tracks
		 (params->instrument_count,
		  notelib_internals_sizeof_instrument(params->inline_step_count, params->reserved_inline_state_space),
		  params->internal_dual_buffer_size)
		 + params->track_count
		   * notelib_internals_sizeof_track
		     (params->queued_command_count+1, //adding 1 because of the design flaw that one buffer element is always introduced
		      params->reserved_inline_initialized_channel_buffer_size + (params->reserved_inline_initialized_channel_buffer_size>0)); /*adding 1 because of the design flaw that one buffer element is always introduced*/}

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

enum notelib_status notelib_internals_deinit(notelib_state_handle state_handle){
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
	// general setup
	struct notelib_processing_step_entry* steps_end = steps + step_count; //end iterator for steps array
	bool at_back = false; //whether the current channel is at the back, moving backwards, looking for active channels to replace the dead front with
	void* current_channel_state; //the channel that is currently being queried
	void* previous_channel_state_back; //cache for unifying branches

	// this once was a do-while-true with custom break points, but with two entry points it's not really worth it
	loop_start_at_front: //initialization reused as assignment in loop entry
		current_channel_state = channel_state_front;
	loop_start_general:;
		// first generate the samples by calling every step's step function
		struct notelib_processing_step_entry* current_step = steps;
		notelib_sample_uint produced_samples = samples_requested;
		notelib_sample* filled_channel_mix_buffer = NULL; //the first step receives NULL as input buffer
		do{
			notelib_sample_uint newly_produced_samples =
				current_step->step
				(filled_channel_mix_buffer, channel_mix_buffer,
				 produced_samples, NOTELIB_INTERNAL_OFFSET_AND_CAST(current_channel_state, current_step->data_offset, void*));
			produced_samples = MIN(produced_samples, newly_produced_samples);
			++current_step;
		if(current_step == steps_end) break;
			filled_channel_mix_buffer = channel_mix_buffer; //further channels receive the channel_mix_buffer as input buffer
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
					cleanup(NOTELIB_INTERNAL_OFFSET_AND_CAST(current_channel_state, current_step->data_offset, void*));
				++current_step;
			}while(current_step != steps_end);
	if(channel_state_front == channel_state_back) return; //end if this was the last channel to handle, f.e. because we just reached the (already inactive) front again
			// move to the back, because the front is now inactive
			current_channel_state = channel_state_back;
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

void cautiously_advance_track_position
(struct notelib_track* track_ptr, notelib_position* new_position, notelib_sample_uint* new_sample_offset, notelib_position old_position, notelib_sample_uint old_position_sample_offset,
 bool* should_check_for_updates, notelib_sample_uint samples_to_process_commands_for){
	*new_sample_offset = old_position_sample_offset + samples_to_process_commands_for;
	notelib_sample_uint tempo_interval = notelib_track_get_tempo_interval(track_ptr, old_position, old_position+1);
	if(*new_sample_offset >= tempo_interval){
		notelib_position a = notelib_track_get_position_change(track_ptr, old_position, *new_sample_offset);
		*new_position = old_position + a;
		notelib_sample_uint b = notelib_track_get_tempo_interval(track_ptr, old_position, *new_position);
		*new_sample_offset -= b;
		track_ptr->position_sample_offset = *new_sample_offset;
		track_ptr->position = *new_position;
		*should_check_for_updates = true;
	}
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
#if NOTELIB_INTERNAL_USE_EXPLICIT_MEMSET
	memset(instrument_mix_buffer, 0, sizeof(notelib_sample)*dual_buffer_size);
#else//#if NOTELIB_INTERNAL_USE_EXPLICIT_MEMSET
	for(uint16_t i = 0; i < dual_buffer_size; ++i)
		instrument_mix_buffer[i] = 0;
#endif//#if NOTELIB_INTERNAL_USE_EXPLICIT_MEMSET
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
		struct notelib_track* track_ptr = notelib_internals_get_track(internals, track_index);
		if(track_ptr->tempo_ceil_interval_samples == 0)
			continue;
		bool should_check_for_updates = false;
		notelib_position old_position = track_ptr->position;
		notelib_position new_position = old_position;
		notelib_sample_uint old_position_sample_offset = track_ptr->position_sample_offset;
		notelib_sample_uint new_sample_offset;
		cautiously_advance_track_position(track_ptr, &new_position, &new_sample_offset, old_position, old_position_sample_offset, &should_check_for_updates, samples_to_process_commands_for);
		should_check_for_updates |= (old_position_sample_offset == 0);
		if(should_check_for_updates){
			struct circular_buffer* command_queue_ptr = notelib_track_get_command_queue(track_ptr);
			const struct notelib_command* command_ptr;
			do{
				command_ptr = circular_buffer_direct_read_commence(command_queue_ptr);
			if(command_ptr == NULL) break;
				notelib_position ceiled_old_position = old_position + (old_position_sample_offset > 0);
				notelib_position new_position_ceiled = new_position + (new_sample_offset > 0);
				notelib_position command_position = command_ptr->position;
				if(command_position >= ceiled_old_position && command_position < new_position_ceiled){
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
						notelib_sample_uint samples_leftover = new_sample_offset;
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
					case notelib_command_type_trigger:;
						const struct notelib_command_trigger* command_trigger = &command_ptr->trigger;
						command_trigger->trigger_function(command_trigger->userdata);
						break;
					}
					circular_buffer_direct_read_commit(command_queue_ptr);
				}else
					break;
			}while(true);
		}
	}
#if NOTELIB_INTERNAL_USE_INTERMEDIATE_MIXING_BUFFER
	#if NOTELIB_INTERNAL_USE_EXPLICIT_MEMCPY
	memcpy(out, instrument_mix_buffer, sizeof(notelib_sample)*samples_requested);
	#else//#if NOTELIB_INTERNAL_USE_EXPLICIT_MEMCPY
	for(notelib_sample_uint sample_index = 0; sample_index < samples_requested; ++sample_index)
		out[sample_index] = instrument_mix_buffer[sample_index];
	#endif//#if NOTELIB_INTERNAL_USE_EXPLICIT_MEMCPY
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
