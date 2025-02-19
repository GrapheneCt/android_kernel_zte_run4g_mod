//===- Signals.cpp - Generic Unix Signals Implementation -----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines some helpful functions for dealing with the possibility of
// Unix signals occurring while your program is running.
//
//===----------------------------------------------------------------------===//

#include "Unix.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/Mutex.h"
#include <algorithm>
#include <string>
#include <vector>
#if HAVE_EXECINFO_H
# include <execinfo.h>         // For backtrace().
#endif
#if HAVE_SIGNAL_H
#include <signal.h>
#endif
#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#if HAVE_CXXABI_H
#include <cxxabi.h>
#endif
#if HAVE_DLFCN_H
#include <dlfcn.h>
#endif
#if HAVE_MACH_MACH_H
#include <mach/mach.h>
#endif

using namespace llvm;

static RETSIGTYPE SignalHandler(int Sig);  // defined below.

static SmartMutex<true> SignalsMutex;

/// InterruptFunction - The function to call if ctrl-c is pressed.
static void (*InterruptFunction)() = 0;

static std::vector<std::string> FilesToRemove;
static std::vector<std::pair<void(*)(void*), void*> > CallBacksToRun;

// IntSigs - Signals that represent requested termination. There's no bug
// or failure, or if there is, it's not our direct responsibility. For whatever
// reason, our continued execution is no longer desirable.
static const int IntSigs[] = {
  SIGHUP, SIGINT, SIGPIPE, SIGTERM, SIGUSR1, SIGUSR2
};
static const int *const IntSigsEnd = array_endof(IntSigs);

// KillSigs - Signals that represent that we have a bug, and our prompt
// termination has been ordered.
static const int KillSigs[] = {
  SIGILL, SIGTRAP, SIGABRT, SIGFPE, SIGBUS, SIGSEGV, SIGQUIT
#ifdef SIGSYS
  , SIGSYS
#endif
// Mono uses SIGXCPU as for GC handling. Since the Jazz method-based JIT is
// powered by LLVM, including this signal here will potentially block the apps
// built on the top of Mono framework because LLVM doesn't chain the termination
// signal (see SignalHandler() in this file below.)
#if !defined(ANDROID)
#ifdef SIGXCPU
  , SIGXCPU
#endif
#endif // !defined(ANDROID)
#ifdef SIGXFSZ
  , SIGXFSZ
#endif
#ifdef SIGEMT
  , SIGEMT
#endif
};
static const int *const KillSigsEnd = array_endof(KillSigs);

static unsigned NumRegisteredSignals = 0;
static struct {
  struct sigaction SA;
  int SigNo;
} RegisteredSignalInfo[(sizeof(IntSigs)+sizeof(KillSigs))/sizeof(KillSigs[0])];


static void RegisterHandler(int Signal) {
  assert(NumRegisteredSignals <
         sizeof(RegisteredSignalInfo)/sizeof(RegisteredSignalInfo[0]) &&
         "Out of space for signal handlers!");

  struct sigaction NewHandler;

  NewHandler.sa_handler = SignalHandler;
  NewHandler.sa_flags = SA_NODEFER|SA_RESETHAND;
  sigemptyset(&NewHandler.sa_mask);

  // Install the new handler, save the old one in RegisteredSignalInfo.
  sigaction(Signal, &NewHandler,
            &RegisteredSignalInfo[NumRegisteredSignals].SA);
  RegisteredSignalInfo[NumRegisteredSignals].SigNo = Signal;
  ++NumRegisteredSignals;
}

static void RegisterHandlers() {
#if !defined(ANDROID)
  // If the handlers are already registered, we're done.
  if (NumRegisteredSignals != 0) return;

  std::for_each(IntSigs, IntSigsEnd, RegisterHandler);
  std::for_each(KillSigs, KillSigsEnd, RegisterHandler);
#endif
}

static void UnregisterHandlers() {
  // Restore all of the signal handlers to how they were before we showed up.
  for (unsigned i = 0, e = NumRegisteredSignals; i != e; ++i)
    sigaction(RegisteredSignalInfo[i].SigNo,
              &RegisteredSignalInfo[i].SA, 0);
  NumRegisteredSignals = 0;
}


/// RemoveFilesToRemove - Process the FilesToRemove list. This function
/// should be called with the SignalsMutex lock held.
/// NB: This must be an async signal safe function. It cannot allocate or free
/// memory, even in debug builds.
static void RemoveFilesToRemove() {
  // We avoid iterators in case of debug iterators that allocate or release
  // memory.
  for (unsigned i = 0, e = FilesToRemove.size(); i != e; ++i) {
    // We rely on a std::string implementation for which repeated calls to
    // 'c_str()' don't allocate memory. We pre-call 'c_str()' on all of these
    // strings to try to ensure this is safe.
    const char *path = FilesToRemove[i].c_str();

    // Get the status so we can determine if it's a file or directory. If we
    // can't stat the file, ignore it.
    struct stat buf;
    if (stat(path, &buf) != 0)
      continue;

    // If this is not a regular file, ignore it. We want to prevent removal of
    // special files like /dev/null, even if the compiler is being run with the
    // super-user permissions.
    if (!S_ISREG(buf.st_mode))
      continue;

    // Otherwise, remove the file. We ignore any errors here as there is nothing
    // else we can do.
    unlink(path);
  }
}

// SignalHandler - The signal handler that runs.
static RETSIGTYPE SignalHandler(int Sig) {
  // Restore the signal behavior to default, so that the program actually
  // crashes when we return and the signal reissues.  This also ensures that if
  // we crash in our signal handler that the program will terminate immediately
  // instead of recursing in the signal handler.
  UnregisterHandlers();

  // Unmask all potentially blocked kill signals.
  sigset_t SigMask;
  sigfillset(&SigMask);
  sigprocmask(SIG_UNBLOCK, &SigMask, 0);

  SignalsMutex.acquire();
  RemoveFilesToRemove();

  if (std::find(IntSigs, IntSigsEnd, Sig) != IntSigsEnd) {
    if (InterruptFunction) {
      void (*IF)() = InterruptFunction;
      SignalsMutex.release();
      InterruptFunction = 0;
      IF();        // run the interrupt function.
      return;
    }

    SignalsMutex.release();
    raise(Sig);   // Execute the default handler.
    return;
  }

  SignalsMutex.release();

  // Otherwise if it is a fault (like SEGV) run any handler.
  for (unsigned i = 0, e = CallBacksToRun.size(); i != e; ++i)
    CallBacksToRun[i].first(CallBacksToRun[i].second);

#ifdef __s390__
  // On S/390, certain signals are delivered with PSW Address pointing to
  // *after* the faulting instruction.  Simply returning from the signal
  // handler would continue execution after that point, instead of
  // re-raising the signal.  Raise the signal manually in those cases.
  if (Sig == SIGILL || Sig == SIGFPE || Sig == SIGTRAP)
    raise(Sig);
#endif
}

void llvm::sys::RunInterruptHandlers() {
  SignalsMutex.acquire();
  RemoveFilesToRemove();
  SignalsMutex.release();
}

void llvm::sys::SetInterruptFunction(void (*IF)()) {
  SignalsMutex.acquire();
  InterruptFunction = IF;
  SignalsMutex.release();
  RegisterHandlers();
}

// RemoveFileOnSignal - The public API
bool llvm::sys::RemoveFileOnSignal(StringRef Filename,
                                   std::string* ErrMsg) {
  SignalsMutex.acquire();
  std::string *OldPtr = FilesToRemove.empty() ? 0 : &FilesToRemove[0];
  FilesToRemove.push_back(Filename);

  // We want to call 'c_str()' on every std::string in this vector so that if
  // the underlying implementation requires a re-allocation, it happens here
  // rather than inside of the signal handler. If we see the vector grow, we
  // have to call it on every entry. If it remains in place, we only need to
  // call it on the latest one.
  if (OldPtr == &FilesToRemove[0])
    FilesToRemove.back().c_str();
  else
    for (unsigned i = 0, e = FilesToRemove.size(); i != e; ++i)
      FilesToRemove[i].c_str();

  SignalsMutex.release();

  RegisterHandlers();
  return false;
}

// DontRemoveFileOnSignal - The public API
void llvm::sys::DontRemoveFileOnSignal(StringRef Filename) {
  SignalsMutex.acquire();
  std::vector<std::string>::reverse_iterator RI =
    std::find(FilesToRemove.rbegin(), FilesToRemove.rend(), Filename);
  std::vector<std::string>::iterator I = FilesToRemove.end();
  if (RI != FilesToRemove.rend())
    I = FilesToRemove.erase(RI.base()-1);

  // We need to call c_str() on every element which would have been moved by
  // the erase. These elements, in a C++98 implementation where c_str()
  // requires a reallocation on the first call may have had the call to c_str()
  // made on insertion become invalid by being copied down an element.
  for (std::vector<std::string>::iterator E = FilesToRemove.end(); I != E; ++I)
    I->c_str();

  SignalsMutex.release();
}

/// AddSignalHandler - Add a function to be called when a signal is delivered
/// to the process.  The handler can have a cookie passed to it to identify
/// what instance of the handler it is.
void llvm::sys::AddSignalHandler(void (*FnPtr)(void *), void *Cookie) {
  CallBacksToRun.push_back(std::make_pair(FnPtr, Cookie));
  RegisterHandlers();
}


// PrintStackTrace - In the case of a program crash or fault, print out a stack
// trace so that the user has an indication of why and where we died.
//
// On glibc systems we have the 'backtrace' function, which works nicely, but
// doesn't demangle symbols.
void llvm::sys::PrintStackTrace(FILE *FD) {
#if defined(HAVE_BACKTRACE) && defined(ENABLE_BACKTRACES)
  static void* StackTrace[256];
  // Use backtrace() to output a backtrace on Linux systems with glibc.
  int depth = backtrace(StackTrace,
                        static_cast<int>(array_lengthof(StackTrace)));
#if HAVE_DLFCN_H && HAVE_CXXABI_H && __GNUG__
  int width = 0;
  for (int i = 0; i < depth; ++i) {
    Dl_info dlinfo;
    dladdr(StackTrace[i], &dlinfo);
    const char* name = strrchr(dlinfo.dli_fname, '/');

    int nwidth;
    if (name == NULL) nwidth = strlen(dlinfo.dli_fname);
    else              nwidth = strlen(name) - 1;

    if (nwidth > width) width = nwidth;
  }

  for (int i = 0; i < depth; ++i) {
    Dl_info dlinfo;
    dladdr(StackTrace[i], &dlinfo);

    fprintf(FD, "%-2d", i);

    const char* name = strrchr(dlinfo.dli_fname, '/');
    if (name == NULL) fprintf(FD, " %-*s", width, dlinfo.dli_fname);
    else              fprintf(FD, " %-*s", width, name+1);

    fprintf(FD, " %#0*lx",
            (int)(sizeof(void*) * 2) + 2, (unsigned long)StackTrace[i]);

    if (dlinfo.dli_sname != NULL) {
      fputc(' ', FD);
#  if HAVE_CXXABI_H
      int res;
      char* d = abi::__cxa_demangle(dlinfo.dli_sname, NULL, NULL, &res);
#  else
      char* d = NULL;
#  endif
      if (d == NULL) fputs(dlinfo.dli_sname, FD);
      else           fputs(d, FD);
      free(d);

      // FIXME: When we move to C++11, use %t length modifier. It's not in
      // C++03 and causes gcc to issue warnings. Losing the upper 32 bits of
      // the stack offset for a stack dump isn't likely to cause any problems.
      fprintf(FD, " + %u",(unsigned)((char*)StackTrace[i]-
                                     (char*)dlinfo.dli_saddr));
    }
    fputc('\n', FD);
  }
#else
  backtrace_symbols_fd(StackTrace, depth, STDERR_FILENO);
#endif
#endif
}

static void PrintStackTraceSignalHandler(void *) {
  PrintStackTrace(stderr);
}

/// PrintStackTraceOnErrorSignal - When an error signal (such as SIGABRT or
/// SIGSEGV) is delivered to the process, print a stack trace and then exit.
void llvm::sys::PrintStackTraceOnErrorSignal() {
  AddSignalHandler(PrintStackTraceSignalHandler, 0);

#if defined(__APPLE__) && !defined(ANDROID)
  // Environment variable to disable any kind of crash dialog.
  if (getenv("LLVM_DISABLE_CRASH_REPORT")) {
    mach_port_t self = mach_task_self();

    exception_mask_t mask = EXC_MASK_CRASH;

    kern_return_t ret = task_set_exception_ports(self,
                             mask,
                             MACH_PORT_NULL,
                             EXCEPTION_STATE_IDENTITY | MACH_EXCEPTION_CODES,
                             THREAD_STATE_NONE);
    (void)ret;
  }
#endif
}


/***/

// On Darwin, raise sends a signal to the main thread instead of the current
// thread. This has the unfortunate effect that assert() and abort() will end up
// bypassing our crash recovery attempts. We work around this for anything in
// the same linkage unit by just defining our own versions of the assert handler
// and abort.

#if defined(__APPLE__) && !defined(ANDROID)

#include <signal.h>
#include <pthread.h>

int raise(int sig) {
  return pthread_kill(pthread_self(), sig);
}

void __assert_rtn(const char *func,
                  const char *file,
                  int line,
                  const char *expr) {
  if (func)
    fprintf(stderr, "Assertion failed: (%s), function %s, file %s, line %d.\n",
            expr, func, file, line);
  else
    fprintf(stderr, "Assertion failed: (%s), file %s, line %d.\n",
            expr, file, line);
  abort();
}

void abort() {
  raise(SIGABRT);
  usleep(1000);
  __builtin_trap();
}

#endif
