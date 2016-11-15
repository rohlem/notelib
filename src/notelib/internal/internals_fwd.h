#ifndef NOTELIB_INTERNAL_INTERNALS_FWD_H_
#define NOTELIB_INTERNAL_INTERNALS_FWD_H_

#include <stdalign.h>

#define MAX(A, B) ((A) > (B) ? (A) : (B))
#define ALIGNAS_MAX(A, B) alignas(MAX(alignof(A), alignof(B)))

struct notelib_internals;

#endif//#ifndef NOTELIB_INTERNAL_INTERNALS_FWD_H_
