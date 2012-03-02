nFLAGS+= -O2 -Wall -ggdb -D_GNU_SOURCE -D__OSX__ -I. -I..
LDADD+=  -L/usr/X11/lib/ -lX11

PREFIX?= /usr/local
BINDIR?= $(PREFIX)/bin
MANDIR?= $(PREFIX)/share/man

CC= gcc

all: sscrotwm

sscrotwm.c:
	ln -sf ../sscrotwm.c

sscrotwm: sscrotwm.o osx.o
	$(CC) $(LDFLAGS) -o $@ $+ $(LDADD)

install: all
	install -m 755 -d $(DESTDIR)$(BINDIR)
	install -m 755 -d $(DESTDIR)$(MANDIR)/man1
	install -m 755 sscrotwm $(DESTDIR)$(BINDIR)
	install -m 644 ../sscrotwm.1 $(DESTDIR)$(MANDIR)/man1/sscrotwm.1

clean:
	rm -f sscrotwm *.o sscrotwm.c

.PHONY: all install clean
