#ifndef NOTELIB_INTERNAL_INTERNALS_FWD_H_
#define NOTELIB_INTERNAL_INTERNALS_FWD_H_

#include <stdalign.h>

#define MAX(A, B) ((A) > (B) ? (A) : (B))
#define MAX3(A, B, C) (MAX(MAX(A, B), C))
#define ALIGNAS_MAX(A, B) alignas(MAX(alignof(A), alignof(B)))
#define ALIGNAS_MAX3(A, B, C) alignas(MAX3(alignof(A), alignof(B), alignof(C)))

struct notelib_internals;

#endif//#ifndef NOTELIB_INTERNAL_INTERNALS_FWD_H_
