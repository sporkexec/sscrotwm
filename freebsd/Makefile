.sinclude <bsd.port.pre.mk>

PREFIX?= /usr/local
LOCALBASE?= /usr/local

SWM_BINDIR?= $(PREFIX)/bin
SWM_MANDIR?= $(PREFIX)/man

BUILDVERSION!= sh "${.CURDIR}/../buildver.sh"
.if !${BUILDVERSION} == ""
CFLAGS+= -DSSCROTWM_BUILDSTR=\"$(BUILDVERSION)\"
.endif

CFLAGS+= -Wall -Wno-uninitialized -I. -I${LOCALBASE}/include
LDADD+= -lutil -lX11 -lXrandr


all: sscrotwm

sscrotwm.c:
	ln -sf ../sscrotwm.c
	ln -sf ../version.h

sscrotwm: sscrotwm.o
	$(CC) $(LDFLAGS) $(LDADD) -o ${.TARGET} ${.ALLSRC}

install: all
	install -m 755 -d $(SWM_BINDIR)
	install -m 755 -d $(SWM_MANDIR)/man1
	install -m 755 sscrotwm $(SWM_BINDIR)
	install -m 644 ../sscrotwm.1 $(SWM_MANDIR)/man1/sscrotwm.1
	ln -sf $(SWM_BINDIR)/sscrotwm $(SWM_BINDIR)/scrotwm

clean:
	rm -f sscrotwm *.o sscrotwm.c version.h

.sinclude <bsd.port.post.mk>
