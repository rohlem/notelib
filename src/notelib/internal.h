#ifndef NOTELIB_INTERNAL_H_
#define NOTELIB_INTERNAL_H_

//TODO (general): Define constants once instead of making them static!

//TODO: Additional feature: triggering specified notelib_processing_step_function-s at specified positions; requires a slight restructuring of the fill_buffer routine and some referencing mechanism (probably id) spawned at play_note
//TODO: Additional feature: global (notelib_internal-level) (and maybe instrument-level) processing-step-style functions for "shader-like" effects etc.
//TODO: (eventually) register exit_handler with atexit

#ifndef NOTELIB_INTERNAL_USE_INTERMEDIATE_MIXING_BUFFER
#define NOTELIB_INTERNAL_USE_INTERMEDIATE_MIXING_BUFFER 0
#endif//#ifndef NOTELIB_INTERNAL_USE_INTERMEDIATE_MIXING_BUFFER

#ifndef NOTELIB_INTERNAL_USE_EXPLICIT_MEMSET
#define NOTELIB_INTERNAL_USE_EXPLICIT_MEMSET 1
#endif//#ifndef NOTELIB_INTERNAL_USE_EXPLICIT_MEMSET

#ifndef NOTELIB_INTERNAL_USE_EXPLICIT_MEMCPY
#define NOTELIB_INTERNAL_USE_EXPLICIT_MEMCPY 1
#endif//#ifndef NOTELIB_INTERNAL_USE_EXPLICIT_MEMCPY

#include "notelib.h"
#include "internal/alignment_utilities.h"
#include "internal/instrument.h"
#include "internal/internals.h"
#include "internal/processing_step_entry.h"
#include "internal/track.h"

#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>

#endif//#ifndef NOTELIB_INTERNAL_H_
