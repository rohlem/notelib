#ifndef NOTELIB_INTERNAL_INSTRUMENT_H_
#define NOTELIB_INTERNAL_INSTRUMENT_H_

#include "../notelib.h"
#include "internals_fwd.h"
#include "processing_step_entry.h"

#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>

#define NOTELIB_INSTRUMENT_STATE_DATA_ALIGNMENT alignof(max_align_t)

//"abstract" (not usable solely on its own) "base"/common type of notelib_instrument_inline_steps and notelib_instrument_external_steps
struct notelib_instrument{
	alignas(NOTELIB_INSTRUMENT_STATE_DATA_ALIGNMENT)
	_Atomic notelib_channel_uint active_channel_count;
	notelib_channel_uint channel_count;
	notelib_instrument_state_uint channel_state_size;
	notelib_step_uint step_count;
	//union of struct notelib_processing_step_entry* steps and struct notelib_processing_step_entry inline_steps[] ; size in no way enforced
	//union of void* external_state and unsigned char inline_state[] ; size in no way enforced
};
//this one is assumed to have the strictest alignment requirement out of all of them
struct notelib_instrument_inline_steps{
	struct notelib_instrument base;
	struct notelib_processing_step_entry inline_steps[];
	//union of void* external_state and unsigned char inline_state[] at &inline_steps[step_count] ; size in no way enforced
};
struct notelib_instrument_external_steps{
	struct notelib_instrument base;
	struct notelib_processing_step_entry* external_steps;
	//union of void* external_state and unsigned char inline_state[] ; size in no way enforced
};
struct notelib_instrument_external_steps_inline_state{
	struct notelib_instrument_external_steps base;
	struct notelib_processing_step_entry* external_steps;
	alignas(NOTELIB_INSTRUMENT_STATE_DATA_ALIGNMENT) unsigned char inline_state[];
};
struct notelib_instrument_external_steps_external_state{
	struct notelib_instrument_external_steps base;
	struct notelib_processing_step_entry* external_steps;
	void* external_state;
};

static const size_t notelib_instrument_offsetof_steps_array          = offsetof(struct notelib_instrument_inline_steps,                    inline_steps);
static const size_t notelib_instrument_offsetof_steps_ptr            = offsetof(struct notelib_instrument_external_steps,                external_steps);
static const size_t notelib_instrument_external_offsetof_state_array = offsetof(struct notelib_instrument_external_steps_inline_state,     inline_state);
static const size_t notelib_instrument_external_offsetof_state_ptr   = offsetof(struct notelib_instrument_external_steps_external_state, external_state);

struct notelib_instrument* notelib_internals_get_instrument(struct notelib_internals* internals, notelib_instrument_uint index);

size_t notelib_instrument_get_inline_steps_size(uint8_t inline_step_count);

size_t notelib_internals_sizeof_instrument_inline_steps_up_to_inline_state_data(uint8_t inline_step_count);

size_t notelib_internals_sizeof_instrument_external_steps_up_to_inline_state_data(uint8_t inline_step_count);

size_t notelib_internals_sizeof_instrument_inline_steps_up_to_external_state_data_ptr(uint8_t inline_step_count);

size_t notelib_internals_sizeof_instrument_external_steps_up_to_external_state_data_ptr(uint8_t inline_step_count);

size_t notelib_internals_sizeof_instrument(uint8_t inline_step_count, uint16_t reserved_inline_state_space);

bool notelib_instrument_are_processing_steps_inline(const struct notelib_internals* internals, const struct notelib_instrument* instrument);

struct notelib_processing_step_entry* notelib_instrument_get_processing_steps(const struct notelib_internals* internals, struct notelib_instrument* instrument);

size_t notelib_internals_effective_sizeof_instrument_up_to_inline_state_data(const struct notelib_internals* internals, struct notelib_instrument* instrument);

size_t notelib_internals_effective_sizeof_instrument_up_to_external_state_data_ptr(const struct notelib_internals* internals, struct notelib_instrument* instrument);

size_t notelib_instrument_get_effective_state_data_inline_space(const struct notelib_internals* internals, struct notelib_instrument* instrument);

bool notelib_instrument_is_state_data_inline(const struct notelib_internals* internals, struct notelib_instrument* instrument);

void*  notelib_instrument_get_inline_state_data_after_inline_steps(const struct notelib_internals* internals, struct notelib_instrument* instrument);

void** notelib_instrument_get_external_state_data_after_inline_steps(const struct notelib_internals* internals, struct notelib_instrument* instrument);

void* notelib_instrument_get_state_data(const struct notelib_internals* internals, struct notelib_instrument* instrument);

void** notelib_instrument_get_external_state_data_ptr(const struct notelib_internals* internals, struct notelib_instrument* instrument, bool processing_steps_inline);

#endif//#ifndef NOTELIB_INTERNAL_INSTRUMENT_H_
