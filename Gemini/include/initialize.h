// initialize.h
// Startup / initialization routines.
// Perl subs mapped:
//   initialize()        → initialize()
//   set_path()          → set_path()
//   set_user_paths()    → set_user_paths()
//   set_basics()        → set_basics()
//   set_system_files()  → set_system_files()
//   set_os()            → set_os()
//   set_display_size()  → set_display_size()
//   set_sep()           → set_sep()
//   set_sudo()          → set_sudo()
//   set_xorg_log()      → set_xorg_log()
//   cleanup()           → cleanup()
#pragma once
#include "global_state.h"

namespace inxi {

// Top-level initialization (called before anything else)
void initialize(GlobalState& state);

// Sub-steps of initialize():
void set_path(GlobalState& state);
void set_user_paths(GlobalState& state);
void set_basics(GlobalState& state);
void set_system_files(GlobalState& state);
void set_os(GlobalState& state);
void set_display_size(GlobalState& state);
void set_sep(GlobalState& state);
void set_sudo(GlobalState& state);
void set_xorg_log(GlobalState& state);

// Called at exit to flush logs / temp files
void cleanup(GlobalState& state);

}  // namespace inxi
