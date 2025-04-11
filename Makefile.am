## Process this file with automake to produce Makefile.in.

# This file was created by copying from PCRE2 and editing. There may still be
# unwanted stuff left over.

AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = -I m4

## The first of these is necessary for building in non-source directory. The
## second is the recommended way of passing DATADIR to the source, since it
## can be modified at make time.

AM_CPPFLAGS=-I$(srcdir)/src -DDATADIR=$(datadir)

## Specify the documentation files that are distributed.

dist_doc_DATA = \
  AUTHORS \
  COPYING \
  ChangeLog \
  LICENCE \
  NEWS \
  README

dist_html_DATA = \
  doc/html/README.txt \
  doc/html/index.html \
  doc/html/b2pf-config.html \
  doc/html/b2pf.html \
  doc/html/b2pftest.html

dist_man_MANS = \
  doc/b2pf-config.1 \
  doc/b2pf.3 \
  doc/b2pftest.1

# Unit tests you want to run when people type 'make check'.
# TESTS is for binary unit tests, check_SCRIPTS for script-based tests

TESTS =
check_SCRIPTS =
dist_noinst_SCRIPTS =

# Some of the binaries we make are to be installed, and others are
# (non-user-visible) helper programs needed to build the libraries.

bin_PROGRAMS =
noinst_PROGRAMS =

# Additional files to delete on 'make clean', 'make distclean',
# and 'make maintainer-clean'.

CLEANFILES =
DISTCLEANFILES = src/config.h.in~
MAINTAINERCLEANFILES =

# Additional files to bundle with the distribution, over and above what
# the Autotools include by default.

EXTRA_DIST =

# Handle the supplied rules files

rules_files = rules/Arabic \
              rules/Arabic-LigExtra

rulesdir = $(datadir)/b2pf/rules
rules_DATA = $(rules_files)
EXTRA_DIST += $(rules_files)

# These files contain additional m4 macros that are used by autoconf.

EXTRA_DIST += \
  m4/ax_pthread.m4 m4/b2pf_visibility.m4

# These files are used in the preparation of a release

EXTRA_DIST += \
  PrepareRelease \
  CheckMan \
  Detrail \
  132html \
  autogen.sh \
  doc/index.html.src

# These files are usable versions of b2pf.h and config.h that are distributed
# for the benefit of people who are building B2PF manually, without the
# Autotools support.

EXTRA_DIST += \
  src/b2pf.h.generic \
  src/config.h.generic

# This is needed because these distributed files depend on built files, and
# the default is to grumble at such things.

distcleancheck_listfiles = src/b2pf.h.generic src/config.h.generic

# The only difference between b2pf.h.in and b2pf.h is the setting of the
# version number. Therefore, we can create the generic version just by copying.

src/b2pf.h.generic: src/b2pf.h.in configure.ac
	rm -f $@
	cp -p src/b2pf.h $@

# It is more complicated for config.h.generic. We need the version that results
# from a default configuration so as to get all the default values. We can get
# this by doing a configure in a temporary directory. However, some trickery is
# needed, because the source directory may already be configured. If you just
# try running configure in a new directory, it complains. For this reason, we
# move config.status out of the way while doing the default configuration. The
# resulting config.h is munged by perl to put #ifdefs round any #defines for
# macros with values, and to #undef all boolean macros such as HAVE_xxx and
# SUPPORT_xxx. We also get rid of any gcc-specific visibility settings. Make
# sure that B2PF_EXP_DEFN is unset (in case it has visibility settings).

src/config.h.generic: configure.ac
	rm -rf $@ _generic
	mkdir _generic
	cs=$(srcdir)/config.status; test ! -f $$cs || mv -f $$cs $$cs.aside
	cd _generic && $(abs_top_srcdir)/configure || :
	cs=$(srcdir)/config.status; test ! -f $$cs.aside || mv -f $$cs.aside $$cs
	test -f _generic/src/config.h
	perl -n \
	  -e 'BEGIN{$$blank=0;}' \
	  -e 'if(/B2PF_EXP_DEFN/){print"/* #undef B2PF_EXP_DEFN */\n";$$blank=0;next;}' \
	  -e 'if(/to make a symbol visible/){next;}' \
	  -e 'if(/__attribute__ \(\(visibility/){next;}' \
	  -e 'if(/LT_OBJDIR/){print"/* This is ignored unless you are using libtool. */\n";}' \
	  -e 'if(/^#define\s((?:HAVE|SUPPORT|STDC)_\w+)/){print"/* #undef $$1 */\n";$$blank=0;next;}' \
	  -e 'if(/^#define\s(?!PACKAGE|VERSION)(\w+)/){print"#ifndef $$1\n$$_#endif\n";$$blank=0;next;}' \
	  -e 'if(/^\s*$$/){print unless $$blank; $$blank=1;} else{print;$$blank=0;}' \
	  _generic/src/config.h >$@
	rm -rf _generic

MAINTAINERCLEANFILES += src/b2pf.h.generic src/config.h.generic

# These are the header files we'll install. We do not distribute b2pf.h
# because it is generated from b2pf.h.in.

nodist_include_HEADERS = src/b2pf.h
include_HEADERS =

# This is the "config" script.

bin_SCRIPTS = b2pf-config

## Define the library to install and the list of sources.

lib_LTLIBRARIES = libb2pf.la
libb2pf_la_SOURCES = \
  src/b2pf.c \
  src/b2pf_context.c \
  src/b2pf_error.c \
  src/b2pf_format.c \
  src/b2pf_internal.h \
  src/b2pf_tree.c \
  src/b2pf_valid_utf.c

libb2pf_la_CFLAGS = \
  $(VISIBILITY_CFLAGS) \
  $(AM_CFLAGS)
libb2pf_la_LIBADD =

libb2pf_la_LDFLAGS = $(EXTRA_LIBB2PF_LDFLAGS)

# Build the general b2pftest program.

bin_PROGRAMS += b2pftest
b2pftest_SOURCES = src/b2pftest.c
b2pftest_CFLAGS = $(AM_CFLAGS)
b2pftest_LDADD = $(LIBREADLINE)
b2pftest_LDADD += libb2pf.la

## The main library tests. Each test is a binary plus a script that runs that
## binary in various ways. We install these test binaries in case folks find it
## helpful. The two .bat files are for running the tests under Windows.

TESTS += RunTest
dist_noinst_SCRIPTS += RunTest

EXTRA_DIST += \
  testdata/testinput0 \
  testdata/testinput1 \
  testdata/testinput2 \
  testdata/testinput3 \
  testdata/testoutput0 \
  testdata/testoutput1 \
  testdata/testoutput2 \
  testdata/testoutput3

# RunTest should clean up after itself, but just in case it doesn't, add its
# working files to CLEANFILES.

CLEANFILES += \
        testtry \
        teststdout \
	teststderr

## ------------ End of testing -------------


# We have .pc files for pkg-config users.

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libb2pf.pc

## end Makefile.am
