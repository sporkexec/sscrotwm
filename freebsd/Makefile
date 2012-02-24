.sinclude <bsd.port.pre.mk>

PREFIX?= /usr/local
LOCALBASE?= /usr/local

SWM_BINDIR?= $(PREFIX)/bin
SWM_LIBDIR?= $(PREFIX)/lib
SWM_MANDIR?= $(PREFIX)/man

LVERS!=	awk -F = '/major/ { printf( "%s.", $$2 ) } /minor/ { printf( "%s", $$2 ) }' ${.CURDIR}/../lib/shlib_version
BUILDVERSION!= sh "${.CURDIR}/../buildver.sh"
.if !${BUILDVERSION} == ""
CFLAGS+= -DSSCROTWM_BUILDSTR=\"$(BUILDVERSION)\"
.endif

CFLAGS+= -Wall -Wno-uninitialized -I. -I${LOCALBASE}/include
CFLAGS+= -DSWM_LIB=\"$(SWM_LIBDIR)/libswmhack.so.$(LVERS)\"
LDADD+= -lutil -L${LOCALBASE}/lib -lX11 -lXrandr -lXtst


all: sscrotwm libswmhack.so.$(LVERS)

sscrotwm.c:
	ln -sf ../sscrotwm.c
	ln -sf ../version.h

swm_hack.c:
	ln -sf ../lib/swm_hack.c

sscrotwm: sscrotwm.o
	$(CC) $(LDFLAGS) $(LDADD) -o ${.TARGET} ${.ALLSRC}

swm_hack.so: swm_hack.c
	$(CC) $(CFLAGS) -c -shared -fpic -DPIC -o ${.TARGET} ${.ALLSRC}

libswmhack.so.$(LVERS): swm_hack.so
	$(CC) $(LDFLAGS) -shared -fpic -o ${.TARGET} ${.ALLSRC}

install: all
	install -m 755 -d $(SWM_BINDIR)
	install -m 755 -d $(SWM_LIBDIR)
	install -m 755 -d $(SWM_MANDIR)/man1
	install -m 755 sscrotwm $(SWM_BINDIR)
	install -m 755 libswmhack.so.$(LVERS) $(SWM_LIBDIR)
	install -m 644 ../sscrotwm.1 $(SWM_MANDIR)/man1/sscrotwm.1
	ln -sf $(SWM_BINDIR)/sscrotwm $(SWM_BINDIR)/scrotwm

clean:
	rm -f sscrotwm *.o *.so libswmhack.so.* sscrotwm.c swm_hack.c version.h

.sinclude <bsd.port.post.mk>
