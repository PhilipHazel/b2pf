dnl Process this file with autoconf to produce a configure script.

dnl NOTE FOR MAINTAINERS: Do not use minor version numbers 08 or 09 because
dnl the leading zeros may cause them to be treated as invalid octal constants
dnl if a B2PF user writes code that uses B2PF_MINOR as a number. There is now
dnl a check further down that throws an error if 08 or 09 are used.

dnl The B2PF_PRERELEASE feature is for identifying release candidates. It might
dnl be defined as -RC2, for example. For real releases, it should be empty.

m4_define(b2pf_major, [0])
m4_define(b2pf_minor, [10])
m4_define(b2pf_prerelease, [])
m4_define(b2pf_date, [2020-09-23])

# Libtool shared library interface versions (current:revision:age)
m4_define(libb2pf_version, [1:0:1])

AC_PREREQ(2.57)
AC_INIT(B2PF, b2pf_major.b2pf_minor[]b2pf_prerelease, , b2pf)
AC_CONFIG_SRCDIR([src/b2pf.h.in])
AM_INIT_AUTOMAKE
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
AC_CONFIG_HEADERS(src/config.h)

# This was added at the suggestion of libtoolize (03-Jan-10)
AC_CONFIG_MACRO_DIR([m4])

# The default CFLAGS in Autoconf are "-g -O2" for gcc and just "-g" for any
# other compiler. There doesn't seem to be a standard way of getting rid of the
# -g (which I don't think is needed for a production library). This fudge seems
# to achieve the necessary. First, we remember the externally set values of
# CFLAGS. Then call the AC_PROG_CC macro to find the compiler - if CFLAGS is
# not set, it will be set to Autoconf's defaults. Afterwards, if the original
# values were not set, remove the -g from the Autoconf defaults.

remember_set_CFLAGS="$CFLAGS"

AC_PROG_CC
AM_PROG_CC_C_O
AC_USE_SYSTEM_EXTENSIONS

if test "x$remember_set_CFLAGS" = "x"
then
  if test "$CFLAGS" = "-g -O2"
  then
    CFLAGS="-O2"
  elif test "$CFLAGS" = "-g"
  then
    CFLAGS=""
  fi
fi

# This is a new thing required to stop a warning from automake 1.12
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])

# Check for a 64-bit integer type
AC_TYPE_INT64_T

AC_PROG_INSTALL
AC_LIBTOOL_WIN32_DLL
LT_INIT
AC_PROG_LN_S

# Check for GCC visibility feature

B2PF_VISIBILITY

# Check for Clang __attribute__((uninitialized)) feature

AC_MSG_CHECKING([for __attribute__((uninitialized))])
AC_LANG_PUSH([C])
tmp_CFLAGS=$CFLAGS
CFLAGS="$CFLAGS -Werror"
AC_COMPILE_IFELSE([AC_LANG_PROGRAM(,
                   [[char buf[128] __attribute__((uninitialized));(void)buf]])],
                   [b2pf_cc_cv_attribute_uninitialized=yes],
                   [b2pf_cc_cv_attribute_uninitialized=no])
AC_MSG_RESULT([$b2pf_cc_cv_attribute_uninitialized])
if test "$b2pf_cc_cv_attribute_uninitialized" = yes; then
  AC_DEFINE([HAVE_ATTRIBUTE_UNINITIALIZED], 1, [Define this if your compiler
             supports __attribute__((uninitialized))])
fi
CFLAGS=$tmp_CFLAGS
AC_LANG_POP([C])

# Versioning

B2PF_MAJOR="b2pf_major"
B2PF_MINOR="b2pf_minor"
B2PF_PRERELEASE="b2pf_prerelease"
B2PF_DATE="b2pf_date"

if test "$B2PF_MINOR" = "08" -o "$B2PF_MINOR" = "09"
then
  echo "***"
  echo "*** Minor version number $B2PF_MINOR must not be used. ***"
  echo "*** Use only 00 to 07 or 10 onwards, to avoid octal issues. ***"
  echo "***"
  exit 1
fi

AC_SUBST(B2PF_MAJOR)
AC_SUBST(B2PF_MINOR)
AC_SUBST(B2PF_PRERELEASE)
AC_SUBST(B2PF_DATE)

# Set a more sensible default value for $(htmldir).
if test "x$htmldir" = 'x${docdir}'
then
  htmldir='${docdir}/html'
fi

# Handle --enable-b2pftest-libedit
AC_ARG_ENABLE(b2pftest-libedit,
              AS_HELP_STRING([--enable-b2pftest-libedit],
                             [link b2pftest with libedit]),
              , enable_b2pftest_libedit=no)

# Handle --enable-b2pftest-libreadline
AC_ARG_ENABLE(b2pftest-libreadline,
              AS_HELP_STRING([--enable-b2pftest-libreadline],
                             [link b2pftest with libreadline]),
              , enable_b2pftest_libreadline=no)

# Handle --enable-valgrind
#AC_ARG_ENABLE(valgrind,
#              AS_HELP_STRING([--enable-valgrind],
#                             [enable valgrind support]),
#              , enable_valgrind=no)

AH_TOP([
/* B2PF is written in Standard C. In environments that support the GNU
autotools, config.h.in is converted into config.h by the "configure" script.
If you are going to build B2PF "by hand" without using "configure", you should
copy the distributed config.h.generic to config.h, and edit the macro
definitions to be the way you need them. You must then add -DHAVE_CONFIG_H to
all of your compile commands, so that config.h is included at the start of
every source.

Alternatively, you can avoid editing by using -D on the compiler command line
to set the macro values. In this case, you do not have to set -DHAVE_CONFIG_H,
but if you do, default values will be taken from config.h for non-boolean
macros that are not defined on the command line.

Boolean macros such as HAVE_STDLIB_H should either be defined (conventionally
to 1) for TRUE, and not defined at all for FALSE. All such macros are listed as
a commented #undef in config.h.generic. Macros whose actual value is relevant
have defaults defined, but are surrounded by #ifndef/#endif lines so that the
value can be overridden by -D. */])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS(limits.h sys/types.h sys/stat.h dirent.h)
AC_CHECK_HEADERS([windows.h], [HAVE_WINDOWS_H=1])

# Checks for typedefs, structures, and compiler characteristics.

AC_C_CONST
AC_TYPE_SIZE_T

# Checks for library functions. Is this really necessary?

AC_CHECK_FUNCS(strerror)

# Check for the availabiity of libreadline

if test "$enable_b2pftest_libreadline" = "yes"; then
 AC_CHECK_HEADERS([readline/readline.h], [HAVE_READLINE_H=1])
 AC_CHECK_HEADERS([readline/history.h], [HAVE_HISTORY_H=1])
 AC_CHECK_LIB([readline], [readline], [LIBREADLINE="-lreadline"],
   [unset ac_cv_lib_readline_readline;
    AC_CHECK_LIB([readline], [readline], [LIBREADLINE="-ltinfo"],
     [unset ac_cv_lib_readline_readline;
      AC_CHECK_LIB([readline], [readline], [LIBREADLINE="-lcurses"],
       [unset ac_cv_lib_readline_readline;
        AC_CHECK_LIB([readline], [readline], [LIBREADLINE="-lncurses"],
         [unset ac_cv_lib_readline_readline;
	  AC_CHECK_LIB([readline], [readline], [LIBREADLINE="-lncursesw"],
           [unset ac_cv_lib_readline_readline;
	    AC_CHECK_LIB([readline], [readline], [LIBREADLINE="-ltermcap"],
             [LIBREADLINE=""],
             [-ltermcap])],
           [-lncursesw])],
         [-lncurses])],
       [-lcurses])],
     [-ltinfo])])
 AC_SUBST(LIBREADLINE)
 if test -n "$LIBREADLINE"; then
   if test "$LIBREADLINE" != "-lreadline"; then
     echo "-lreadline needs $LIBREADLINE"
     LIBREADLINE="-lreadline $LIBREADLINE"
   fi
 fi
fi


# Check for the availability of libedit. Different distributions put its
# headers in different places. Try to cover the most common ones.

if test "$enable_b2pftest_libedit" = "yes"; then
  AC_CHECK_HEADERS([editline/readline.h], [HAVE_EDITLINE_READLINE_H=1],
    [AC_CHECK_HEADERS([edit/readline/readline.h], [HAVE_READLINE_READLINE_H=1],
      [AC_CHECK_HEADERS([readline/readline.h], [HAVE_READLINE_READLINE_H=1])])])
  AC_CHECK_LIB([edit], [readline], [LIBEDIT="-ledit"])
fi

B2PF_STATIC_CFLAG=""
if test "x$enable_shared" = "xno" ; then
  AC_DEFINE([B2PF_STATIC], [1], [
    Define to any value if linking statically (TODO: make nice with Libtool)])
  B2PF_STATIC_CFLAG="-DB2PF_STATIC"
fi
AC_SUBST(B2PF_STATIC_CFLAG)

# Here is where B2PF-specific defines are handled

if test "$enable_b2pftest_libedit" = "yes"; then
  AC_DEFINE([SUPPORT_LIBEDIT], [], [
    Define to any value to allow b2pftest to be linked with libedit.])
  LIBREADLINE="$LIBEDIT"
elif test "$enable_b2pftest_libreadline" = "yes"; then
  AC_DEFINE([SUPPORT_LIBREADLINE], [], [
    Define to any value to allow b2pftest to be linked with libreadline.])
fi

# Platform specific issues
NO_UNDEFINED=
EXPORT_ALL_SYMBOLS=
case $host_os in
  cygwin* | mingw* )
    if test X"$enable_shared" = Xyes; then
      NO_UNDEFINED="-no-undefined"
      EXPORT_ALL_SYMBOLS="-Wl,--export-all-symbols"
    fi
    ;;
esac

# Extra LDFLAGS for the library. The libb2pf*_version values are m4 variables,
# assigned above.

EXTRA_LIBB2PF_LDFLAGS="$EXTRA_LIBB2PF_LDFLAGS \
  $NO_UNDEFINED -version-info libb2pf_version"

AC_SUBST(EXTRA_LIBB2PF_LDFLAGS)

# When we run 'make distcheck', use these arguments. Turning off compiler
# optimization makes it run faster.
DISTCHECK_CONFIGURE_FLAGS="CFLAGS=''"
AC_SUBST(DISTCHECK_CONFIGURE_FLAGS)

# Checks for --enable-b2pftest-readline

if test "$enable_b2pftest_libedit" = "yes"; then
  if test "$enable_b2pftest_libreadline" = "yes"; then
    echo "** Cannot use both --enable-b2pftest-libedit and --enable-b2pftest-readline"
    exit 1
  fi
  if test "$HAVE_EDITLINE_READLINE_H" != "1" -a \
          "$HAVE_READLINE_READLINE_H" != "1"; then
    echo "** Cannot --enable-b2pftest-libedit because neither editline/readline.h"
    echo "** nor readline/readline.h was found."
    exit 1
  fi
  if test -z "$LIBEDIT"; then
    echo "** Cannot --enable-b2pftest-libedit because libedit library was not found."
    exit 1
  fi
fi

if test "$enable_b2pftest_libreadline" = "yes"; then
  if test "$HAVE_READLINE_H" != "1"; then
    echo "** Cannot --enable-b2pftest-readline because readline/readline.h was not found."
    exit 1
  fi
  if test "$HAVE_HISTORY_H" != "1"; then
    echo "** Cannot --enable-b2pftest-readline because readline/history.h was not found."
    exit 1
  fi
  if test -z "$LIBREADLINE"; then
    echo "** Cannot --enable-b2pftest-readline because readline library was not found."
    exit 1
  fi
fi

# Produce these files, in addition to config.h.

AC_CONFIG_FILES(
	Makefile
	libb2pf.pc
	b2pf-config
	src/b2pf.h
)

# Make the generated script files executable.

AC_CONFIG_COMMANDS([script-chmod], [chmod a+x b2pf-config])

AC_OUTPUT

# Print out a nice little message after configure is run displaying the
# chosen options.

cat <<EOF

$PACKAGE-$VERSION configuration summary:

    Install prefix ..................... : ${prefix}
    C preprocessor ..................... : ${CPP}
    C compiler ......................... : ${CC}
    Linker ............................. : ${LD}
    C preprocessor flags ............... : ${CPPFLAGS}
    C compiler flags ................... : ${CFLAGS} ${VISIBILITY_CFLAGS}
    Linker flags ....................... : ${LDFLAGS}
    Extra libraries .................... : ${LIBS}

    Build shared libs .................. : ${enable_shared}
    Build static libs .................. : ${enable_static}
    Link b2pftest with libedit ......... : ${enable_b2pftest_libedit}
    Link b2pftest with libreadline ..... : ${enable_b2pftest_libreadline}

EOF

dnl end configure.ac
