.include <bsd.xconf.mk>

PREFIX?=/usr/local

BINDIR=${PREFIX}/bin

PROG=sscrotwm
MAN=sscrotwm.1

CFLAGS+=-std=c89 -Wall -Wno-uninitialized -ggdb3
CPPFLAGS+= -I${X11BASE}/include
LDADD+=-lutil -L${X11BASE}/lib -lX11
BUILDVERSION != sh "${.CURDIR}/buildver.sh"
.if !${BUILDVERSION} == ""
CPPFLAGS+= -DSSCROTWM_BUILDSTR=\"$(BUILDVERSION)\"
.endif

MANDIR= ${PREFIX}/man/man

obj: _xenocara_obj

# clang targets
.if ${.TARGETS:M*analyze*}
CC=clang
CXX=clang++
CPP=clang -E
CFLAGS+=--analyze
.elif ${.TARGETS:M*clang*}
CC=clang
CXX=clang++
CPP=clang -E
.endif

analyze: all
clang: all
.include <bsd.prog.mk>
.include <bsd.xorg.mk>
