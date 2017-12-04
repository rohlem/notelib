**[on hiatus for at least a couple of months into 2018; no upper limit]**

***NOTE: master branch is outdated (missing some backported fixes), most recent development was on branch 'immediate-track'***

(There's not a lot of documentation though, good luck getting either of them to work...)

**Current status: R0 (first release); on hiatus (2 months minimum, 12 months expected, no upper limit)**

Notelib is a C11 library intended to abstract asynchronous communication of a main ("client") thread and an audio thread that generates waveform representations of "notes" the client thread issued.

---

The basic structure looks as follows:

 a) The notelib internals are initialized at a given position; this can be done when the audio backend is initialized, and hidden from the client program as desired.
 
 b) Instruments are registered that define the way waveforms are generated.
 
 c) A track is started with a given tempo and notes (and change tempo commands) are placed on it at given positions. Currently notes must be submitted sequentially.
 
Currently this repository includes a rudimentary first version of notelib, as well as an example program ("main.c") including a minimal backend wrapper/"connector" ("back.h" and "back.c") for PortAudio (excluding that library - please get that yourself) and a couple manually-verifiable test cases (in "test").

---

**"Cricital" bugs (things that need to be fixed to guarantee it works correctly):**
 - struct notelib_track.tempo_ceil_interval_samples (used as an "enabled"-flag) is currently accessed from the audio thread without a cache-refreshing acquire-read, which is technically totally unsafe. Just add some way to implement that; possibly a "track enabling queue" or something useless like that.
   (note how in contrast, everything else is only accessed when a respective command is read through a track's command queue, which involves such an acquire read operation.)

**"Non-critical" to-dos (fixes/enhancements that would seriously improve the library interface):**
 - separate all components declared and defined in notelib/internal.h (currently it contains near everything)
 - separate declarations and definitions of everything in notelib/internal.h (currently that header must be included in exactly one translation unit, which is obviously an inhuman crime)
 - clean up the "main interface header" ("notelib/notelib.h") to only include what you actually want it to (and not overlap with any "backend connectors")
 - restructure the repository (put interface headers into an "include" directory, translation units for compilation into "src" or something, separate "backend connectors" (if you even want them) and examples into an "examples" or "test" directory or similar)
 - remove or replace the random commented out debug printf calls that are still floating around
 - document the interface of components (like structs) a little better (using functions to group init - and deinit routines, interpreting state as "enabled"-flags, making naming more consistent etc.)
 - adding a way to block on interaction with notelib (my attempt would be to shod together a C11 interface for C++11's std::condition_variable)
 - add further configuration options with macros and enums and stuff (there's probably a lot you _could_ do)
 - some minor optimizations probably, though honestly I don't think that's a big worry

(- rewrite it in C++ to be more configurable in less than a fourth of its current code size ._.) (though I'm actually quite happy with how small its memory footprint turned out to be -u-)
