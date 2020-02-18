#! /bin/sh
:
#ident	"@(#)newsyslog:mksigname.sh:$Format:%D:%ci:%cN:%h$"

#
# Script to generate sys_signame.c from siglist.in
#
# The intput filename can be given as the first parameter, and the
# output filename given as the second parameter.  If only one
# parameter is given it is used as the input filename.  By default
# input and output are STDIN and STDOUT.
#
# The technique for doing this comes from the siglist.sh script
# included in the PD-KSH sources, apparently written by Michael
# Rendell <michael@panda.cs.mun.ca>

set -e

tmpin="tmpi$$.c"
tmpout="tmpo$$.c"
trapsigs='0 1 2 13 15'

if [ $# -gt 0 ]; then
	siglist_file=$1
	shift
	exec < $siglist_file
else
	siglist_file="<standard-input>"
fi

if [ $# -gt 0 ]; then
	outfile=$1
	shift
	exec > $outfile
else
	outfile="<standard-output>"
fi


trap 'rc=$?; rm -f $tmpin $tmpout; trap 0; exit $rc' $trapsigs

CC="${CC:-cc}"
CPP="${CPP:-${CC} -E -C}"

cat <<__EOF__
/*
 * $outfile:
 *
 *	define an array listing the names of system signals
 *
 * #     #                                                   ###
 * #  #  #    ##    #####   #    #     #    #    #   ####    ###
 * #  #  #   #  #   #    #  ##   #     #    ##   #  #    #   ###
 * #  #  #  #    #  #    #  # #  #     #    # #  #  #         #
 * #  #  #  ######  #####   #  # #     #    #  # #  #  ###
 * #  #  #  #    #  #   #   #   ##     #    #   ##  #    #   ###
 *  ## ##   #    #  #    #  #    #     #    #    #   ####    ###
 *
 * THIS FILE IS GENERATED AUTOMATICALLY BY THE SCRIPT $0 FROM
 * THE SIGNAL LIST IN THE FILE "$siglist_file".
 *
 * IF YOU SEE ANY "Unknown Signal" ENTRIES YOU MUST MAKE ADDITIONS TO
 * THE SIGNAL LIST FILE AND REBUILD RATHER THAN EDITING THIS FILE
 * DIRECTLY.
 */

#include "config.h"
#include <sys/types.h>

__EOF__

(
	echo '#include <signal.h>';
	# note sed reads siglist.in from stdin
	sed -e '/^[	 ]*#/d' \
	    -e '/^[ 	]*$/d' \
	    -e 's/^[	 ]*\([^ 	][^ 	]*\)[	 ][	 ]*\(.*[^ 	]\)[ 	]*$/#ifdef SIG\1\
	QwErTy SIG\1	"\1",   	\/\* \2 \*\/\
#endif/'
) > $tmpin

$CPP $CPPFLAGS $tmpin > $tmpout

sed -n -e 's|"\(..\)", /|"\1",		/|' \
       -e 's|"\(...\)", /|"\1",		/|' \
       -e 's|"\(..*\)", /|"\1",	/|' \
       -e 's|^[ 	]*QwErTy \([0-9]\)[ 	]|/*  \1 */ |p' \
       -e 's|^[ 	]*QwErTy \([0-9][0-9]*\)[ 	]|/* \1 */ |p' < $tmpout | \
	sort -n +1 | \
	awk 'BEGIN {
		last = 0;
		nsigs = 0;
		printf("const char *const sys_signame[] = {\n");
		printf("/*  0 */ \"Signal 0\",\t/* Fake value for zero */\n");
	}
	{
		n = $2;
		if (n > 0 && n != last) {
			while (++last < n) {
				printf("/* %2d */ \"#%d\",\t\t/* Unknown Signal %d !!! */\n", last, last, last);
			}
			last = n;
			print $0;
		}
	}
	END {
		printf("};\n\n");
		printf("const unsigned int sys_nsigname = sizeof(sys_signame) / sizeof(sys_signame[0]);\n");
	}'

exit 0
