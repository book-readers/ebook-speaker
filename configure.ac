#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([ebook-speaker], [6.1], [jos@jlemmens.nl], [], [www.jlemmens.nl])
AM_INIT_AUTOMAKE([subdir-objects])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
# FIXME: Replace `main' with a function in `-lmagic':
AC_CHECK_LIB([magic], [main])
# FIXME: Replace `main' with a function in `-lncursesw':
AC_CHECK_LIB([ncursesw], [main])
# FIXME: Replace `main' with a function in `-lsox':
AC_CHECK_LIB([sox], [main])
# FIXME: Replace `main' with a function in `-lxml2':
AC_CHECK_LIB([xml2], [main])
# FIXME: Replace `main' with a function in `-lpulse':
AC_CHECK_LIB([pulse], [main])
# FIXME: Replace `main' with a function in `-lasound':
AC_CHECK_LIB([asound], [main])

AM_SILENT_RULES([yes])

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h libintl.h locale.h stdlib.h string.h strings.h \
                  sys/mount.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_PID_T
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_STRTOD
AC_CHECK_FUNCS([strstr])
AC_CHECK_FUNCS([atexit isascii mkdir select setlocale strcasecmp strchr strdup strerror strrchr decode_entities])
AC_CHECK_FUNCS([bzero])
AC_CHECK_FUNCS([rmdir])
AC_CHECK_FUNCS([strncasecmp])

AC_CONFIG_FILES([Makefile
                 m4/Makefile
                 man/Makefile
                 doc/Makefile
                 po/Makefile.in
                 src/Makefile
                 src/pactl/Makefile])
                 
AM_INIT_AUTOMAKE
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.18.3])
AC_PROG_LN_S
AC_CONFIG_SUBDIRS([src/pactl])
AC_OUTPUT                    

dnl pulseaudio/pactl stuff

echo
echo "Configure pulseaudio/pactl..."
echo

(cd src/pactl && ./configure)
