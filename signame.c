/*	$NetBSD: signame.c,v 1.9 1998/11/30 20:42:44 thorpej Exp $	*/

/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "from: @(#)siglist.c	5.6 (Berkeley) 2/23/91";*/
#else
__RCSID("$NetBSD: signame.c,v 1.9 1998/11/30 20:42:44 thorpej Exp $");
#endif
#endif /* LIBC_SCCS and not lint */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#if !defined(SYS_SIGNAME_DECLARED)
const char *const sys_signame[] = {
	"Signal 0",
	"HUP",		/* SIGHUP */
	"INT",		/* SIGINT */
	"QUIT",		/* SIGQUIT */
	"ILL",		/* SIGILL */
	"TRAP",		/* SIGTRAP */
	"ABRT",		/* SIGABRT */
	"EMT",		/* SIGEMT */
	"FPE",		/* SIGFPE */
	"KILL",		/* SIGKILL */
	"BUS",		/* SIGBUS */
	"SEGV",		/* SIGSEGV */
	"SYS",		/* SIGSYS */
	"PIPE",		/* SIGPIPE */
	"ALRM",		/* SIGALRM */
	"TERM",		/* SIGTERM */
	"URG",		/* SIGURG */
	"STOP",		/* SIGSTOP */
	"TSTP",		/* SIGTSTP */
	"CONT",		/* SIGCONT */
	"CHLD",		/* SIGCHLD */
	"TTIN",		/* SIGTTIN */
	"TTOU",		/* SIGTTOU */
	"IO",		/* SIGIO */
	"XCPU",		/* SIGXCPU */
	"XFSZ",		/* SIGXFSZ */
	"VTALRM",	/* SIGVTALRM */
	"PROF",		/* SIGPROF */
	"WINCH",	/* SIGWINCH */
	"INFO",		/* SIGINFO */
	"USR1",		/* SIGUSR1 */
	"USR2"		/* SIGUSR2 */
};
#endif
