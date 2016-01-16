//----------------------------------------------------------------------------------------------------------------------
// Program     :  main
// Copyright   :  (c) Julian Bouzas 2014
// License     :  BSD3-style (see LICENSE)
// Maintainer  :  Julian Bouzas - nnoell3[at]gmail.com
// Stability   :  stable
//----------------------------------------------------------------------------------------------------------------------


//----------------------------------------------------------------------------------------------------------------------
// PREPROCESSOR
//----------------------------------------------------------------------------------------------------------------------

// Includes
#include "neuro/system.h"


//----------------------------------------------------------------------------------------------------------------------
// VARIABLE DECLARATION
//----------------------------------------------------------------------------------------------------------------------

// FlagHandlerFn
typedef bool (*const FlagHandlerFn)();

// Flag
typedef struct Flag Flag;
struct Flag {
  const char *const name;
  const FlagHandlerFn handler;
  const char *const desc;
};


//----------------------------------------------------------------------------------------------------------------------
// FUNCTION DECLARATION
//----------------------------------------------------------------------------------------------------------------------

// Flag Handlers
static bool help_handler();
static bool version_handler();
static bool recompile_handler();
static bool reload_handler();

// Main
static bool run_neurowm(int argc, const char *const *argv, int *status);
static bool run_flag(const char *flgname);
static bool loop_run_neurowm(int argc, const char *const *argv);


//----------------------------------------------------------------------------------------------------------------------
// VARIABLE DEFINITION
//----------------------------------------------------------------------------------------------------------------------

// Flags                              NAME           HANDLER            DESC
static const Flag help_flag_      = { "--help",      help_handler,      "Print this message"           };
static const Flag version_flag_   = { "--version",   version_handler,   "Print the version number"     };
static const Flag recompile_flag_ = { "--recompile", recompile_handler, "Recompile your configuration" };
static const Flag reload_flag_    = { "--reload",    reload_handler,    "Reload the window manager"    };

// Flags array
static const Flag* flag_list_[] = { &help_flag_, &version_flag_, &recompile_flag_, /*&reload_flag_,*/ NULL };


//----------------------------------------------------------------------------------------------------------------------
// FUNCTION DEFINITION
//----------------------------------------------------------------------------------------------------------------------

static bool help_handler() {
  printf("Usage: neurowm [OPTION]\nOptions:\n");
  int i;
  for (i=0; flag_list_[ i ]; ++i)
    printf("  %s\t\t%s\n", flag_list_[ i ]->name, flag_list_[ i ]->desc);
  return true;
}

static bool version_handler() {
  printf(PKG_NAME " " PKG_VERSION "\n");
  return true;
}

static bool recompile_handler() {
  int pid;
  if (!NeuroSystemSpawn(NeuroSystemGetRecompileCommand(NULL, NULL), &pid))
    return false;
  int status;
  waitpid(pid, &status, WUNTRACED);
  return true;
}

static bool reload_handler() {
  return kill(NeuroSystemGetWmPid(), SIGUSR1) != -1;
}

static bool run_neurowm(int argc, const char *const *argv, int *status) {
  assert(argv);
  const char *cmd[ argc + 1 ];
  NeuroSystemGetRecompileCommand(cmd + 0, NULL);
  cmd[ argc ] = NULL;
  int i;
  for (i = 1; i < argc; ++i)
    cmd[ i ] = argv[ i ];
  int pid;
  if (!NeuroSystemSpawn((const char *const *)cmd, &pid))
    return false;
  waitpid(pid, status, WUNTRACED);
  return true;
}

static bool run_flag(const char *flgname) {
  assert(flgname);
  int i;
  for (i=0; flag_list_[ i ]; ++i)
    if (!strcmp(flgname, flag_list_[ i ]->name)) {
      if (!flag_list_[ i ]->handler())
        perror(flgname);
      return true;
    }
  return false;
}

static bool loop_run_neurowm(int argc, const char *const *argv) {
  assert(argv);
  int status;
  do {
    if (!run_neurowm(argc, argv, &status))
      return false;
  } while (WEXITSTATUS(status) == EXIT_RELOAD);
  return true;
}


//----------------------------------------------------------------------------------------------------------------------
// MAIN DEFINITION
//----------------------------------------------------------------------------------------------------------------------

int main(int argc, const char *const *argv) {
  bool runwm = true, reswm = true;
  if (argc > 1) {
    int i;
    for (i = 1; i < argc; ++i)
      runwm = !run_flag(argv[ i ]);
  }
  if (runwm)
    reswm = loop_run_neurowm(argc, argv);
  return reswm ? EXIT_SUCCESS : EXIT_FAILURE;
}
