#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.67])
AC_INIT([marisa], [0.2.4], [syata@acm.org])
AC_CONFIG_SRCDIR([lib/marisa.h])
AM_INIT_AUTOMAKE

# Checks for programs.
LT_INIT
AC_PROG_CXX
AC_PROG_INSTALL

AC_CONFIG_MACRO_DIR([m4])

# Checks for SSE availability.
AC_MSG_CHECKING([whether to use SSE2])
AC_ARG_ENABLE([sse2],
              [AS_HELP_STRING([--enable-sse2],
                              [use SSE2 [default=no]])],
              [],
              [enable_sse2="no"])
AS_IF([test "x${enable_sse2}" != "xno"], [
  enable_sse2="yes"
])
AC_MSG_RESULT([${enable_sse2}])

AC_MSG_CHECKING([whether to use SSE3])
AC_ARG_ENABLE([sse3],
              [AS_HELP_STRING([--enable-sse3],
                              [use SSE3 [default=no]])],
              [],
              [enable_sse3="no"])
AS_IF([test "x${enable_sse3}" != "xno"], [
  enable_sse3="yes"
])
AC_MSG_RESULT([${enable_sse3}])

AC_MSG_CHECKING([whether to use SSSE3])
AC_ARG_ENABLE([ssse3],
              [AS_HELP_STRING([--enable-ssse3],
                              [use SSSE3 [default=no]])],
              [],
              [enable_ssse3="no"])
AS_IF([test "x${enable_ssse3}" != "xno"], [
  enable_ssse3="yes"
])
AC_MSG_RESULT([${enable_ssse3}])

AC_MSG_CHECKING([whether to use SSE4.1])
AC_ARG_ENABLE([sse4.1],
              [AS_HELP_STRING([--enable-sse4.1],
                              [use SSE4.1 [default=no]])],
              [],
              [enable_sse4_1="no"])
AS_IF([test "x${enable_sse4_1}" != "xno"], [
  enable_sse4_1="yes"
])
AC_MSG_RESULT([${enable_sse4_1}])

AC_MSG_CHECKING([whether to use SSE4.2])
AC_ARG_ENABLE([sse4.2],
              [AS_HELP_STRING([--enable-sse4.2],
                              [use SSE4.2 [default=no]])],
              [],
              [enable_sse4_2="no"])
AS_IF([test "x${enable_sse4_2}" != "xno"], [
  enable_sse4_2="yes"
])
AC_MSG_RESULT([${enable_sse4_2}])

AC_MSG_CHECKING([whether to use SSE4])
AC_ARG_ENABLE([sse4],
              [AS_HELP_STRING([--enable-sse4],
                              [use SSE4 [default=no]])],
              [],
              [enable_sse4="no"])
AS_IF([test "x${enable_sse4}" != "xno"], [
  enable_sse4="yes"
])
AC_MSG_RESULT([${enable_sse4}])

AC_MSG_CHECKING([whether to use SSE4a])
AC_ARG_ENABLE([sse4a],
              [AS_HELP_STRING([--enable-sse4a],
                              [use SSE4a [default=no]])],
              [],
              [enable_sse4a="no"])
AS_IF([test "x${enable_sse4a}" != "xno"], [
  enable_sse4a="yes"
])
AC_MSG_RESULT([${enable_sse4a}])

AC_MSG_CHECKING([whether to use popcnt])
AC_ARG_ENABLE([popcnt],
              [AS_HELP_STRING([--enable-popcnt],
                              [use POPCNT [default=no]])],
              [],
              [enable_popcnt="no"])
AS_IF([test "x${enable_popcnt}" != "xno"], [
  enable_popcnt="yes"
])
AC_MSG_RESULT([${enable_popcnt}])

AS_IF([test "x${enable_popcnt}" != "xno"], [
  enable_sse3="yes"
])
AS_IF([test "x${enable_sse4a}" != "xno"], [
  enable_popcnt="yes"
  enable_sse3="yes"
])
AS_IF([test "x${enable_sse4}" != "xno"], [
  enable_popcnt="yes"
  enable_sse4_2="yes"
])
AS_IF([test "x${enable_sse4_2}" != "xno"], [
  enable_popcnt="yes"
  enable_sse4_1="yes"
])
AS_IF([test "x${enable_sse4_1}" != "xno"], [
  enable_ssse3="yes"
])
AS_IF([test "x${enable_ssse3}" != "xno"], [
  enable_sse3="yes"
])
AS_IF([test "x${enable_sse3}" != "xno"], [
  enable_sse2="yes"
])

AS_IF([test "x${enable_popcnt}" != "xno"], [
  CXXFLAGS="$CXXFLAGS -DMARISA_USE_POPCNT -mpopcnt"
])
if test "x${enable_sse4a}" != "xno"; then
  CXXFLAGS="$CXXFLAGS -DMARISA_USE_SSE4A -msse4a"
elif test "x${enable_sse4}" != "xno"; then
  CXXFLAGS="$CXXFLAGS -DMARISA_USE_SSE4 -msse4"
elif test "x${enable_sse4_2}" != "xno"; then
  CXXFLAGS="$CXXFLAGS -DMARISA_USE_SSE4_2 -msse4.2"
elif test "x${enable_sse4_1}" != "xno"; then
  CXXFLAGS="$CXXFLAGS -DMARISA_USE_SSE4_1 -msse4.1"
elif test "x${enable_ssse3}" != "xno"; then
  CXXFLAGS="$CXXFLAGS -DMARISA_USE_SSSE3 -mssse3"
elif test "x${enable_sse3}" != "xno"; then
  CXXFLAGS="$CXXFLAGS -DMARISA_USE_SSE3 -msse3"
elif test "x${enable_sse2}" != "xno"; then
  CXXFLAGS="$CXXFLAGS -DMARISA_USE_SSE2 -msse2"
fi

AC_CONFIG_FILES([Makefile
                 marisa.pc
                 lib/Makefile
                 tests/Makefile
                 tools/Makefile])
AC_OUTPUT

AS_ECHO([])
AS_ECHO(["${PACKAGE_NAME} ${PACKAGE_VERSION} configuration:"])
AS_ECHO(["-------------------------------"])
AS_ECHO(["  HOST:      ${host}"])
AS_ECHO(["  CXX:       ${CXX}"])
AS_ECHO(["  CXXFLAGS:  ${CXXFLAGS}"])
AS_ECHO(["  LDFLAGS:   ${LDFLAGS}"])
AS_ECHO(["  PREFIX:    ${prefix}"])
AS_ECHO([])
AS_ECHO(["  SSE2:      ${enable_sse2}"])
AS_ECHO(["  SSE3:      ${enable_sse3}"])
AS_ECHO(["  SSSE3:     ${enable_ssse3}"])
AS_ECHO(["  SSE4.1:    ${enable_sse4_1}"])
AS_ECHO(["  SSE4.2:    ${enable_sse4_2}"])
AS_ECHO(["  SSE4a:     ${enable_sse4a}"])
AS_ECHO(["  POPCNT:    ${enable_popcnt}"])
AS_ECHO([])
