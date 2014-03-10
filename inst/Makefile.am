## Process this file with automake to produce Makefile.in

bin_SCRIPTS = linpac
EXTRA_DIST = linpac.in
CLEANFILES = linpac

## replace autoconf variables in scripts with Makefile variables that will
## be resolved at build/install time.
edit = sed \
	-e 's|@bindir[@]|$(bindir)|g' \
	-e 's|@pkgdatadir[@]|$(pkgdatadir)|g' \
	-e 's|@pkglibexecdir[@]|$(pkglibexecdir)|g' \
	-e 's|@prefix[@]|$(prefix)|g'

linpac: Makefile
	rm -f $@ $@.tmp
	srcdir=''; \
	test -f ./$@.in || srcdir=$(srcdir)/; \
	$(edit) $${srcdir}$@.in >$@.tmp

	chmod +x $@.tmp
	chmod a-w $@.tmp
	mv $@.tmp $@

linpac: $(srcdir)/linpac.in

