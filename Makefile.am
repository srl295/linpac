## Process this file with automake to produce Makefile.in

## Automake options should match the init call in configure.ac
AUTOMAKE_OPTIONS = gnu -Wall -Werror -Wportability

## See https://bugzilla.redhat.com/show_bug.cgi?id=901333 for more
## information about ACLOCAL_AMFLAGS
## The work-around for this bug is to commit the m4/ directory.
ACLOCAL_AMFLAGS = -I m4

SUBDIRS = ctt data doc macro mail src inst plugins

dist_pkgdata_DATA = README NEWS
EXTRA_DIST = linpac.lsm COPYING

.PHONY: postinstall noinstall

#install: postinstall

postinstall: 
	(cd $(DESTDIR)@prefix@/share/linpac/bin && rm -f autotx >/dev/null 2>/dev/null)
	(cd $(DESTDIR)@prefix@/share/linpac/bin && @LN_S@ autobin autotx >/dev/null 2>/dev/null)
	(cd $(DESTDIR)@prefix@/share/linpac/bin && rm -f rawtx >/dev/null 2>/dev/null)
	(cd $(DESTDIR)@prefix@/share/linpac/bin && @LN_S@ rawio rawtx >/dev/null 2>/dev/null)
	(cd $(DESTDIR)@prefix@/share/linpac/bin && rm -f yapptx >/dev/null 2>/dev/null)
	(cd $(DESTDIR)@prefix@/share/linpac/bin && @LN_S@ yapp yapptx >/dev/null 2>/dev/null)

noinstall:
	@if [ -x ./src/linpac ]; then \
	(cd src && @LN_S@ applications bin >/dev/null 2>/dev/null); \
	(cd src && @LN_S@ ../macro macro >/dev/null 2>/dev/null); \
	(cd src && @LN_S@ ../mail mail >/dev/null 2>/dev/null); \
	(cd src && mkdir save >/dev/null 2>/dev/null); \
	(cd src && mkdir user >/dev/null 2>/dev/null); \
	(cd src && mkdir log >/dev/null 2>/dev/null); \
	(cd src/applications && @LN_S@ autobin autotx); \
	(cd src/applications && @LN_S@ rawio rawtx); \
	(cd src/applications && @LN_S@ yapp yapptx); \
	(cd src/applications && @LN_S@ mailer/mail mail); \
	cp ctt/*.ctt src; \
	(cd src/applications/liblinpac && make install); \
	echo LinPac is now prepared for running from ./src; \
	else echo Please execute the 'make' command first; \
	fi
