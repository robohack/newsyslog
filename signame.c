/*
 * Copyright (C) 2002 Planix, Inc.  See COPYING for details.
 */

#include <sys/cdefs.h>

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)newsyslog:$Name:  $:$Id: signame.c,v 1.2 2002/01/05 17:58:59 woods Exp $";*/
#endif /* LIBC_SCCS and not lint */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>	/* for BSD define */
#endif
#include <signal.h>

#if !defined(SYS_SIGNAME_DECLARED)
/*
 * List of official signal names, indexed by signal number
 */
const char *const sys_signame[] = {
	"Signal 0",
# ifdef BSD
	"HUP",		/*  1 : SIGHUP */
	"INT",		/*  2 : SIGINT */
	"QUIT",		/*  3 : SIGQUIT */
	"ILL",		/*  4 : SIGILL */
	"TRAP",		/*  5 : SIGTRAP */
	"ABRT",		/*  6 : SIGABRT & SIGIOT */
	"EMT",		/*  7 : SIGEMT */
	"FPE",		/*  8 : SIGFPE */
	"KILL",		/*  9 : SIGKILL */
	"BUS",		/* 10 : SIGBUS */
	"SEGV",		/* 11 : SIGSEGV */
	"SYS",		/* 12 : SIGSYS */
	"PIPE",		/* 13 : SIGPIPE */
	"ALRM",		/* 14 : SIGALRM */
	"TERM",		/* 15 : SIGTERM */
	"URG",		/* 16 : SIGURG */
	"STOP",		/* 17 : SIGSTOP */
	"TSTP",		/* 18 : SIGTSTP */
	"CONT",		/* 19 : SIGCONT */
	"CHLD",		/* 20 : SIGCHLD */
	"TTIN",		/* 21 : SIGTTIN */
	"TTOU",		/* 22 : SIGTTOU */
	"IO",		/* 23 : SIGIO */
	"XCPU",		/* 24 : SIGXCPU */
	"XFSZ",		/* 25 : SIGXFSZ */
	"VTALRM",	/* 26 : SIGVTALRM */
	"PROF",		/* 27 : SIGPROF */
	"WINCH",	/* 28 : SIGWINCH */
	"INFO",		/* 29 : SIGINFO */
	"USR1",		/* 30 : SIGUSR1 */
	"USR2",		/* 31 : SIGUSR2 */
#  ifdef SIGPWR
	"PWR"		/* 32 : SIGPWR */
#  else
	0		/* 32 : not defined */
#  endif
# endif	/* BSD */

# ifdef __linux__
	"HUP",		/*  1 : SIGHUP       Hangup (POSIX).  */
	"INT",		/*  2 : SIGINT       Interrupt (ANSI).  */
	"QUIT",		/*  3 : SIGQUIT      Quit (POSIX).  */
	"ILL",		/*  4 : SIGILL       Illegal instruction (ANSI).  */
	"TRAP",		/*  5 : SIGTRAP      Trace trap (POSIX).  */
	"ABRT",		/*  6 : SIGABRT      Abort (ANSI).  */
	"IOT",		/*  6 : SIGIOT       IOT trap (4.2 BSD).  */
	"BUS",		/*  7 : SIGBUS       BUS error (4.2 BSD).  */
	"FPE",		/*  8 : SIGFPE       Floating-point exception (ANSI).  */
	"KILL",		/*  9 : SIGKILL      Kill, unblockable (POSIX).  */
	"USR1",		/* 10 : SIGUSR1      User-defined signal 1 (POSIX).  */
	"SEGV",		/* 11 : SIGSEGV      Segmentation violation (ANSI).  */
	"USR2",		/* 12 : SIGUSR2      User-defined signal 2 (POSIX).  */
	"PIPE",		/* 13 : SIGPIPE      Broken pipe (POSIX).  */
	"ALRM",		/* 14 : SIGALRM      Alarm clock (POSIX).  */
	"TERM",		/* 15 : SIGTERM      Termination (ANSI).  */
	"STKFLT",	/* 16 : SIGSTKFLT    ??? */
	"CHLD",		/* 17 : SIGCHLD & SIGCLD   Child status has changed (POSIX).  */
	"CONT",		/* 18 : SIGCONT      Continue (POSIX).  */
	"STOP",		/* 19 : SIGSTOP      Stop, unblockable (POSIX).  */
	"TSTP",		/* 20 : SIGTSTP      Keyboard stop (POSIX).  */
	"TTIN",		/* 21 : SIGTTIN      Background read from tty (POSIX).  */
	"TTOU",		/* 22 : SIGTTOU      Background write to tty (POSIX).  */
	"URG",		/* 23 : SIGURG       Urgent condition on socket (4.2 BSD).  */
	"XCPU",		/* 24 : SIGXCPU      CPU limit exceeded (4.2 BSD).  */
	"XFSZ",		/* 25 : SIGXFSZ      File size limit exceeded (4.2 BSD).  */
	"VTALRM",	/* 26 : SIGVTALRM    Virtual alarm clock (4.2 BSD).  */
	"PROF",		/* 27 : SIGPROF      Profiling alarm clock (4.2 BSD).  */
	"WINCH",	/* 28 : SIGWINCH     Window size change (4.3 BSD, Sun).  */
	"IO",		/* 29 : SIGIO & SIGPOLL   I/O now possible (4.2 BSD).  */
	"PWR",		/* 30 : SIGPWR       Power failure restart (System V).  */
	"UNUSED",	/* 31 : unused       */
	0		/* 32 : STUPID LINUX defines NSIG as "Biggest signal number + 1." */
# endif	/* __linux__ */

/* note on SunOS-5.x signals are defined in <iso/signal_iso.h> */

# ifdef __SVR4
	"HUP",		/*  1 : SIGHUP       Exit      Hangup [see termio(7)] */
	"INT",		/*  2 : SIGINT       Exit      Interrupt [see termio(7)] */
	"QUIT",		/*  3 : SIGQUIT      Core      Quit [see termio(7)] */
	"ILL",		/*  4 : SIGILL       Core      Illegal Instruction */
	"TRAP",		/*  5 : SIGTRAP      Core      Trace/Breakpoint Trap */
	"ABRT",		/*  6 : SIGABRT      Core      Abort */
	"EMT",		/*  7 : SIGEMT       Core      Emulation Trap */
	"FPE",		/*  8 : SIGFPE       Core      Arithmetic Exception */
	"KILL",		/*  9 : SIGKILL      Exit      Killed */
	"BUS",		/* 10 : SIGBUS       Core      Bus Error */
	"SEGV",		/* 11 : SIGSEGV      Core      Segmentation Fault */
	"SYS",		/* 12 : SIGSYS       Core      Bad System Call */
	"PIPE",		/* 13 : SIGPIPE      Exit      Broken Pipe */
	"ALRM",		/* 14 : SIGALRM      Exit      Alarm Clock */
	"TERM",		/* 15 : SIGTERM      Exit      Terminated */
	"USR1",		/* 16 : SIGUSR1      Exit      User Signal 1 */
	"USR2",		/* 17 : SIGUSR2      Exit      User Signal 2 */
	"CHLD",		/* 18 : SIGCHLD      Ignore    Child Status */
	"PWR",		/* 19 : SIGPWR       Ignore    Power Fail/Restart */
	"WINCH",	/* 20 : SIGWINCH     Ignore    Window Size Change */
	"URG",		/* 21 : SIGURG       Ignore    Urgent Socket Condition */
	"POLL",		/* 22 : SIGPOLL      Ignore    Socket I/O Possible */
	"STOP",		/* 23 : SIGSTOP      Stop      Stopped (signal) */
	"TSTP",		/* 24 : SIGTSTP      Stop      Stopped (user) [see termio(7)] */
	"CONT",		/* 25 : SIGCONT      Ignore    Continued */
	"TTIN",		/* 26 : SIGTTIN      Stop      Stopped (tty input) [see termio(7)] */
	"TTOU",		/* 27 : SIGTTOU      Stop      Stopped (tty output) [see termio(7)] */
	"VTALRM",	/* 28 : SIGVTALRM    Exit      Virtual Timer Expired */
	"PROF",		/* 29 : SIGPROF      Exit      Profiling Timer Expired */
	"XCPU",		/* 30 : SIGXCPU      Core      CPU time limit exceeded [see getrlimit(2)] */
	"XFSZ",		/* 31 : SIGXFSZ      Core      File size limit exceeded [see getrlimit(2)] */
# endif	/* __SVR4 */

};

/*
 * This table MUST be NSIG + 1 entries long.
 *
 * The magic numbers below must be manually maintained to match the number of
 * entries given above.
 */
# ifdef BSD
#  if (NSIG > 33)
#   include "ERROR: size of sys_signame does not match NSIG!"
#  endif
# endif

# ifdef __linux__
#  if (NSIG > 32)
#   include "ERROR: size of sys_signame does not match NSIG!"
#  endif
# endif

# ifdef __SVR4
#  if (NSIG > 31)
#   include "ERROR: size of sys_signame does not match NSIG!"
#  endif
# endif

# if !(defined(BSD) || defined(__linux__) || defined(__SVR4))
#  include "ERROR: your system is not supported without a native sys_signame"
# endif

#endif /* ! SYS_SIGNAME_DECLARED */
