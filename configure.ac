#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([NAVI Scheme], [0.1])
AC_CONFIG_SRCDIR([navii.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

AC_ARG_WITH([icu], [AS_HELP_STRING([--without-icu],
                                   [configure without unicode support])])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MKDIR_P

# Checks for compiler flags
AX_CHECK_COMPILE_FLAG([-Wall], [CFLAGS+=" -Wall"])
AX_CHECK_COMPILE_FLAG([-Wextra], [CFLAGS+=" -Wextra"])
AX_CHECK_COMPILE_FLAG([-Wpedantic], [CFLAGS+=" -Wpedantic"])
AX_CHECK_COMPILE_FLAG([-Wno-unused-parameter],
                      [CFLAGS+=" -Wno-unused-parameter"])
AX_CHECK_COMPILE_FLAG([-Wno-empty-body], [CFLAGS+=" -Wno-empty-body"])
AX_CHECK_COMPILE_FLAG([-Wno-missing-field-initializers],
                      [CFLAGS+=" -Wno-missing-field-initializers"])
AX_CHECK_COMPILE_FLAG([-std=c11],
                      [CC+=" -std=c11"],
                      [AC_PROG_CC_STDC])

# Checks for libraries.
AS_IF([test "x$with_icu" != "xno"],
      [PKG_CHECK_MODULES([ICU],
                         [icu-uc icu-i18n],
                         [AC_DEFINE([HAVE_ICU],
                                    [1],
                                    [Define to 1 if you have the ICU library]
                         )],
			 [AC_MSG_WARN([configuring without Unicode support])]
      )]
)

# Checks for header files.
AC_CHECK_HEADERS([stddef.h stdint.h stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_INT32_T
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([clock_gettime strcspn strtol])

AC_CONFIG_FILES([makefile])
AC_OUTPUT
