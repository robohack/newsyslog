dnl
dnl locally defined macros for autoconf
dnl

dnl This file:  Copyright (C) 2001 Planix, Inc.
dnl             see COPYING for details

dnl #ident "@(#)newsyslog:$Name:  $:$Id: acinclude.m4,v 1.2 2002/01/04 00:35:15 woods Exp $"

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

dnl ### Checking for function declarations in header files

# AC_CHECK_DECL(DECLARATION,
#		[ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND],
#		[INCLUDES])
AC_DEFUN(AC_CHECK_DECL,
[AC_MSG_CHECKING([whether $1 is declared])
AC_CACHE_VAL(ac_cv_func_decl_$1,
[AC_TRY_COMPILE([$4], [
#ifndef $1
char *p = (char *) $1
#endif
],
       eval "ac_cv_func_decl_$1=yes",
       eval "ac_cv_func_decl_$1=no")])
  if eval "test \"`echo '$ac_cv_func_decl_'$1`\" = yes"; then
    AC_MSG_RESULT(yes)
    ifelse([$2], , :, [$2])
  else
    AC_MSG_RESULT(no)
  ifelse([$3], , , [$3
])dnl
  fi
])dnl

# AC_CHECK_DECLS(SYMBOLS,
#		 [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND],
#		 [INCLUDES])
AC_DEFUN(AC_CHECK_DECLS,
[for ac_func_decl in $1 ; do
  AC_CHECK_DECL($ac_func_decl,
  [changequote(, )dnl
    ac_tr_func_decl=HAVE_DECL_`echo $ac_func_decl | tr 'abcdefghijklmnopqrstuvwxyz' 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'`
changequote([, ])dnl
    AC_DEFINE_UNQUOTED($ac_tr_func_decl, 1) $2],
  [changequote(, )dnl
    ac_tr_func_decl=HAVE_DECL_`echo $ac_func_decl | tr 'abcdefghijklmnopqrstuvwxyz' 'ABCDEFGHIJKLMNOPQRSTUVWXYZ'`
changequote([, ])dnl
    AC_DEFINE_UNQUOTED($ac_tr_func_decl, 0) $3],
  [$4])dnl
done
])
