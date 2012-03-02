CFLAGS+= -Wall -ggdb -D_GNU_SOURCE -I.
LDADD+= -lX11 -lXrandr

PREFIX?= /usr/local
BINDIR?= $(PREFIX)/bin
MANDIR?= $(PREFIX)/share/man

CC= gcc

BUILDVERSION= $(shell sh $(CURDIR)/../buildver.sh)
ifneq ("${BUILDVERSION}", "")
CFLAGS+= -DSSCROTWM_BUILDSTR=\"$(BUILDVERSION)\"
endif

all: sscrotwm

sscrotwm.c:
	ln -sf ../sscrotwm.c
	ln -sf ../version.h

sscrotwm: sscrotwm.o linux.o
	$(CC) $(LDFLAGS) -o $@ $+ $(LDADD)

install: all
	install -m 755 -d $(DESTDIR)$(BINDIR)
	install -m 755 -d $(DESTDIR)$(MANDIR)/man1
	install -m 755 sscrotwm $(DESTDIR)$(BINDIR)
	install -m 644 ../sscrotwm.1 $(DESTDIR)$(MANDIR)/man1/sscrotwm.1

clean:
	rm -f sscrotwm *.o sscrotwm.c version.h

.PHONY: all install clean
