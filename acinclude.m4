dnl
dnl locally defined macros for autoconf
dnl

dnl This file:  Copyright (C) 2001 Planix, Inc.
dnl             see COPYING for details

dnl #ident "@(#)newsyslog:$Name:  $:$Id: acinclude.m4,v 1.3 2002/01/13 19:47:38 woods Exp $"

dnl ### Specific check for declaration of sys_signame in <signal.h>

AC_DEFUN(AC_DECL_SYS_SIGNAME,
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
  AC_DEFINE(SYS_SIGNAME_DECLARED)
fi
])
