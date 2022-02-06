#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([pactl-set-volume], [1.0], [jos@jlemmens.nl], [], [www.jlemmens.nl])
AM_INIT_AUTOMAKE([subdir-objects])
AC_CONFIG_FILES([Makefile
                 m4/Makefile
                 src/Makefile])

AM_SILENT_RULES([yes])

#AC_CONFIG_SRCDIR([ratelimit.c])
#AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
# FIXME: Replace `main' with a function in `-lc':
AC_CHECK_LIB([c], [main])
# FIXME: Replace `main' with a function in `-lg':
AC_CHECK_LIB([g], [main])
# FIXME: Replace `main' with a function in `-lpulse':
AC_CHECK_LIB([pulse], [main])
# FIXME: Replace `main' with a function in `-lsndfile':
AC_CHECK_LIB([sndfile], [main])
AC_CHECK_LIB([cos], [main])

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h inttypes.h langinfo.h libintl.h limits.h locale.h netdb.h stdint.h stdlib.h string.h sys/param.h sys/socket.h syslog.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_UID_T
AC_C_INLINE
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_MODE_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_GETGROUPS
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_STRERROR_R
AC_FUNC_STRTOD
AC_CHECK_FUNCS([getrlimit setrlimit atexit getcwd memset mkdir nl_langinfo realpath regcomp rmdir setenv setlocale socket strcasecmp strchr strcspn strerror strncasecmp strrchr strspn strstr strtol strtoul uname])

AC_CANONICAL_HOST
AC_DEFINE_UNQUOTED([CANONICAL_HOST], "$host", [Canonical host string.])
AC_DEFINE_UNQUOTED(PA_CFLAGS, "$CFLAGS", [The CFLAGS used during compilation])

AC_OUTPUT
