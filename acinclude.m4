dnl
dnl locally defined macros for autoconf
dnl

dnl This file:  Copyright (C) by Planix, Inc.
dnl             see COPYING for details

dnl #ident "@(#)newsyslog:acinclude.m4:$Format:%D:%ci:%cN:%h$"

AC_DEFUN([AC_SOURCE_VERSION], [. $srcdir/$1])

dnl Specific check for declaration of sys_signame in <signal.h>
AC_DEFUN([AC_DECL_SYS_SIGNAME],
[AC_CACHE_CHECK([for sys_signame declaration in signal.h],
  ac_cv_decl_sys_signame,
[AC_TRY_COMPILE([
#include <sys/types.h>
#include <signal.h>
], [
char *msg = *(sys_signame + 1);
],
  ac_cv_decl_sys_signame=yes, ac_cv_decl_sys_signame=no)])
if test $ac_cv_decl_sys_signame = yes; then
  AC_DEFINE(SYS_SIGNAME_DECLARED, [], [Define if <signal.h> declares 'sys_signame'])
fi
])

dnl Specific check for declaration of sys_nsig in <signal.h>
AC_DEFUN([AC_DECL_SYS_NSIG],
[AC_CACHE_CHECK([for sys_nsig declaration in signal.h],
  ac_cv_decl_sys_nsig,
[AC_TRY_COMPILE([
#include <sys/types.h>
#include <signal.h>
], [
int i = (sys_nsig + 1);
],
  ac_cv_decl_sys_nsig=yes, ac_cv_decl_sys_nsig=no)])
if test $ac_cv_decl_sys_nsig = yes; then
  AC_DEFINE(SYS_NSIG_DECLARED, [], [Define if <signal.h> declares 'sys_nsig'])
fi
])

dnl XXX this doesn't work, at least not with GNU LD, unless you static-link
dnl 
dnl dnl AC_REPLACE_C_DEF(IDENTIFIER)
dnl AC _ DEFUN(AC_REPLACE_C_DEF,
dnl [AC_MSG_CHECKING([for definition of $1])
dnl  AC_TRY_LINK([], [extern char *$1; void *foo = &$1;], [AC_MSG_RESULT(yes)], [
dnl   AC_MSG_RESULT(no)
dnl   AC_LIBOBJ([$1])])dnl
dnl ])
