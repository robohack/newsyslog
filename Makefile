#	$Id: Makefile,v 1.2 1997/09/26 21:21:11 woods Exp $

SHELL=	/bin/sh

PROG=	newsyslog
SRCS=	newsyslog.c
OBJS=	newsyslog.o

DEFS=	-DCONF=\"/etc/newsyslog.conf\" \
	-DPIDFILE=\"/etc/syslog.pid\" \
	-DCOMPRESS=\"/usr/ucb/gzip\" \
	-DCOMPRESS_POSTFIX=\".Z\"

CFLAGS=	-O ${DEFS}

LIBS=#	-lnsl

BINOWN=	root
MAN8EXT=8
MAN8=	newsyslog.${MAN8EXT}

${PROG}:	${OBJS}
	${CC} ${CFLAGS} ${OBJS} -o $@ ${LIBS}

clean:
	-rm -f ${PROG} ${OBJS} core
