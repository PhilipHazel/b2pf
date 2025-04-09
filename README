README file for the B2PF (base to presentation form) library
------------------------------------------------------------

B2PF is a library of functions for converting "base" characters in Unicode
strings into appropriate "presentation forms" for display or printing. The
library was inspired by the need to do this for characters in Arabic scripts,
but as it is configurable, it can be used (with an appropriate configuration)
in any situation where this kind of transformation is required. A basic Arabic
configuration is supplied with the distribution.

B2PF supports UTF-8, UTF-16, and UTF-32 strings, and can handle input and
output in reading order or reverse reading order. The library is written in C.
Documentation is provided in the form of man pages, and also as a set of HTML
files.


Building B2PF
-------------

The following instructions assume the use of the widely used "configure; make;
make install" (autotools) process. However, it should be straightforward to
build B2PF on any system that has a Standard C compiler and library, because
it uses only Standard C functions.

To build B2PF on a system that supports autotools, first run the "configure"
command from the B2PF distribution directory, with your current directory set
to the directory where you want the files to be created. This command is a
standard GNU "autoconf" configuration script, for which generic instructions
are supplied in the file INSTALL.

IMPORTANT: There is a limitation in the autotools system that causes some
things not to work if the name of the current directory contains one or more
spaces. To avoid this issue it is strongly recommended that you choose a
directory name without spaces.

Most commonly, people build B2PF within its own distribution directory, and in
this case, on many systems, just running "./configure" is sufficient. However,
the usual methods of changing standard defaults are available. For example:

CFLAGS='-O2 -Wall' ./configure --prefix=/opt/local

This command specifies that the C compiler should be run with the flags '-O2
-Wall' instead of the default, and that "make install" should install B2PF
under /opt/local instead of the default /usr/local.

If you want to build in a different directory, just run "configure" with that
directory as current. For example, suppose you have unpacked the B2PF source
into /source/b2pf/b2pf-xxx, but you want to build it in /build/b2pf/b2pf-xxx:

cd /build/b2pf/b2pf-xxx
/source/b2pf/b2pf-xxx/configure

B2PF is written in C and is normally compiled as a C library. However, it is
possible to build it as a C++ library, though the provided building apparatus
does not have any features to support this.

There are some optional features that can be included or omitted from the B2PF
library.

. The default distribution builds B2PF as a shared library and a static library
  if the operating system supports shared libraries. Shared library support
  relies on the "libtool" script which is built as part of the "configure"
  process.

  The libtool script is used to compile and link both shared and static
  libraries. They are placed in a subdirectory called .libs when they are newly
  built. The program b2pftest is built to use these uninstalled libraries (by
  means of wrapper scripts in the case of shared libraries). When you use "make
  install" to install shared libraries, b2pftest is automatically re-built to
  use the newly installed shared libraries before being installed itself.
  However, the version left in the build directory still uses the uninstalled
  library.

  To build a B2PF static library only you must use --disable-shared when
  configuring it. For example:

  ./configure --prefix=/usr/gnu --disable-shared

  Then run "make" in the usual way. Similarly, you can use --disable-static to
  build only a shared library.

. It is possible to compile b2pftest so that it links with the libreadline
  or libedit libraries, by specifying, respectively,

  --enable-b2pftest-libreadline or --enable-b2pftest-libedit

  If this is done, when b2pftest's input is from a terminal, it reads it using
  the readline() function. This provides line-editing and history facilities.
  Note that libreadline is GPL-licensed, so if you distribute a binary of
  b2pftest linked in this way, there may be licensing issues. These can be
  avoided by linking with libedit (which has a BSD licence) instead.

  Enabling libreadline causes the -lreadline option to be added to the
  b2pftest build. In many operating environments with a sytem-installed
  readline library this is sufficient. However, in some environments (e.g. if
  an unmodified distribution version of readline is in use), it may be
  necessary to specify something like LIBS="-lncurses" as well. This is
  because, to quote the readline INSTALL, "Readline uses the termcap functions,
  but does not link with the termcap or curses library itself, allowing
  applications which link with readline the to choose an appropriate library."
  If you get error messages about missing functions tgetstr, tgetent, tputs,
  tgetflag, or tgoto, this is the problem, and linking with the ncurses library
  should fix it.

The "configure" script builds the following files:

. Makefile             the makefile that builds the library
. src/config.h         build-time configuration options for the library
. src/b2pf.h           the public B2PF header file
. b2pf-config          script that shows the building settings such as CFLAGS
                         that were set for "configure"
. libb2pf.pc           data for the pkg-config command
. libtool              script that builds shared and/or static libraries

Versions of config.h and b2pf.h are distributed in the src directory of B2PF
tarballs under the names config.h.generic and b2pf.h.generic. These are
provided for those who have to build B2PF without using "configure". If you use
"configure", the .generic versions are not used.

The "configure" script also creates config.status, which is an executable
script that can be run to recreate the configuration, and config.log, which
contains compiler output from tests that "configure" runs.

Once "configure" has run, you can run "make". This builds the B2PF library and
a test program called b2pftest. Running "make" with the -j option may speed up
compilation on multiprocessor systems.

The command "make check" runs all the appropriate tests. Details of the B2PF
tests are given below in a separate section of this document. The -j option of
"make" can also be used when running the tests.

You can use "make install" to install B2PF into live directories on your
system. The following are installed (file names are all relative to the
<prefix> that is set when "configure" is run):

  Commands (bin):
    b2pftest
    b2pf-config

  Libraries (lib):
    libb2pf

  Configuration information (lib/pkgconfig):
    libb2pf.pc

  Header files (include):
    b2pf.h

  Man pages (share/man/man{1,3}):
    b2pftest.1
    b2pf-config.1
    b2pf.3

  HTML documentation (share/doc/b2pf/html):
    index.html
    *.html and *.txt (more pages, hyperlinked from index.html)

  Text file documentation (share/doc/b2pf):
    AUTHORS
    COPYING
    ChangeLog
    LICENCE
    NEWS
    README

If you want to remove B2PF from your system, you can run "make uninstall".
This removes all the files that "make install" installed. However, it does not
remove any directories, because these are often shared with other programs.


Retrieving configuration information
------------------------------------

Running "make install" installs the command b2pf-config, which can be used to
recall information about the B2PF configuration and installation. For example:

  b2pf-config --version

prints the version number, and

  b2pf-config --libs

outputs information about where the library is installed. This command
can be included in makefiles for programs that use B2PF, saving the programmer
from having to remember too many details. Run b2pf-config with no arguments to
obtain a list of possible arguments.

The pkg-config command is another system for saving and retrieving information
about installed libraries. For example:

  pkg-config --libs libb2pf

The data is held in *.pc files that are installed in a directory called
<prefix>/lib/pkgconfig.


Cross-compiling using autotools
-------------------------------

You can specify CC and CFLAGS in the normal way to the "configure" command, in
order to cross-compile B2PF for some other host.


Making new tarballs
-------------------

The command "make dist" creates a B2PF tarball in tar.gz format. The command
"make distcheck" does the same, but then does a trial build of the new
distribution to ensure that it works.

If you have modified any of the man page sources in the doc directory, you
should first run the PrepareRelease script before making a distribution. This
script creates the HTML forms of the documentation from the man pages.


Testing B2PF
-------------

To test the basic B2PF library on a Unix-like system, run the RunTest script or
obey "make check", which does the same thing. You can call RunTest with the
single argument "list" to cause it to output a list of its tests.

The RunTest script runs the b2pftest test program (which is documented in its
own man page) on each of the relevant testinput files in the testdata
directory, and compares the output with the contents of the corresponding
testoutput files. RunTest uses a file called testtry to hold the main output
from b2pftest. Other files whose names begin with "test" are used as working
files.

Apart from test 0, which tests error detection while reading B2PF rules, the
entire set of tests is run three times, passing the test strings to the library
as UTF-8, UTF-16, or UTF-32, respectively. If you want to run just one set of
tests, call RunTest with the -8, -16 or -32 option.

If valgrind is installed, you can run the tests under it by putting "-valgrind"
on the RunTest command line. To run b2pftest on just one or more specific test
files, give their numbers as arguments to RunTest, for example:

  RunTest 2 7

You can also specify ranges of tests such as 3-6 or 3- (meaning 3 to the
end), or a number preceded by ~ to exclude a test. For example:

  Runtest 3-7 ~5

This runs tests 3 to 7, excluding test 5, and just ~2 runs all the tests
except test 2. Whatever order the arguments are in, the tests are always run
in numerical order.


File manifest
-------------

The distribution should contain the files listed below.

(A) Source files for the B2PF library functions and their headers are found in
    the src directory, along with the source of the test program:

  src/b2pf.c            )
  src/b2pf_context.c    )
  src/b2pf_error.c      )
  src/b2pf_format.c     ) sources for the library and internal functions
  src/b2pf_tree.c       )
  src/b2pf_valid_utf.c  )
  src/b2pf_internal.h   ) header for internal use

  src/config.h.in       template for config.h, when built by "configure"
  src/b2pf.h.in         template for b2pf.h when built by "configure"

  src/b2pftest.c        source of the b2pftest program

(B) Auxiliary files for building B2PF "by hand"

  src/b2pf.h.generic    ) a version of the public B2PF header file
                        )   for use in non-"configure" environments
  src/config.h.generic  ) a version of config.h for use in non-"configure"
                        )   environments

(C) Distributed rules files

  rules/Arabic          rules for Arabic scripts

(D) Testing script and data files

  RunTest               a Unix shell script for running tests using b2pftest
  testdata/testinput*   test data for library tests
  testdata/testoutput*  expected test results

(E) Documentation

  README                this file
  NEWS                  important changes in this release
  ChangeLog             log of changes to the code
  INSTALL               generic installation instructions
  LICENCE               conditions for the use of B2PF
  COPYING               the same, using GNU's standard name
  doc/*.3               man page sources for B2PF
  doc/*.1               man page sources for b2pftest and b2pf-config
  doc/index.html.src    the base HTML page
  doc/html/*            HTML documentation

(F) Auxiliary files:

  132html               script to turn "man" pages into HTML
  AUTHORS               information about the author of B2PF
  Checkman              script for checking man pages
  Detrail               script to remove trailing spaces
  Makefile.in           ) template for Unix Makefile, which is built by
                        )   "configure"
  Makefile.am           ) the automake input that was used to create
                        )   Makefile.in
  autogen.sh            script to run libtoolize, aclocal, autoconf,
                          autoheader, and finally automake
  PrepareRelease        script to make preparations for "make dist"
  config.guess          ) files used by libtool,
  config.sub            )   used only when building a shared library
  configure             a configuring shell script (built by autoconf)
  configure.ac          ) the autoconf input that was used to build
                        )   "configure" and config.h
  libpb2pf.pc.in        template for libb2pf.pc for pkg-config
  b2pf-config.in        source of script which retains B2PF information

(G) Files generated by or used by the autotools system

  aclocal.m4            m4 macros (generated by "aclocal")
  ar-lib                wrapper for Microsoft lib.exe
  depcomp               ) script to find program dependencies, generated by
                        )   automake
  compile               wrapper for compilers which do not understand '-c -o'
  install-sh            a shell script for installing files
  ltmain.sh             file used to build a libtool script
  m4/*                  m4 files
  test-driver           a test driver script
  missing               ) common stub for a few missing GNU programs while
                        )   installing, generated by automake

Philip Hazel
Email local part: Philip.Hazel
Email domain: gmail.com
Last updated: 09 April 2025
