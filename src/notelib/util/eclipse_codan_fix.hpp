#ifndef NOTELIB_UTIL_ECLIPSE_CODAN_FIX_HPP_
#define NOTELIB_UTIL_ECLIPSE_CODAN_FIX_HPP_

// eclipse cdt's codan (code (static) analysis) doesn't understand some C11 keywords/mechanisms; about 2/3 of errors can be fixed by just undefining/ignoring them:
// (!!!OBVIOUSLY COMMENT THESE BACK OUT BEFORE ACTUALLY BUILDING THE LIBRARY(/ TEST PROGRAMS)!!!)

/*
#include <stdatomic.h>

#define _Atomic
#define _Alignas(ARG)

// for some reason these won't work -.-
#undef atomic_load_explicit
#define atomic_load_explicit(...) 0
#undef atomic_store_explicit
#define atomic_store_explicit(...) 0
*/

#endif//#ifndef NOTELIB_UTIL_ECLIPSE_CODAN_FIX_HPP_
