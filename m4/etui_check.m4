dnl            DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
dnl                    Version 2, December 2004
dnl
dnl Copyright (C) 2004 Sam Hocevar <sam@hocevar.net>
dnl
dnl Everyone is permitted to copy and distribute verbatim or modified
dnl copies of this license document, and changing it is allowed as long
dnl as the name is changed.
dnl
dnl            DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
dnl   TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION
dnl
dnl  0. You just DO WHAT THE FUCK YOU WANT TO.


dnl Copyright (C) 2013 Vincent Torri <vincent dot torri at gmail dot com>
dnl This code is licensed as WTFPL

dnl Macros that check if Etui dependencies are available


dnl use: ETUI_CHECK_DEP_CB(want_static[, ACTION-IF-FOUND[, ACTION-IF-NOT-FOUND]])

AC_DEFUN([ETUI_CHECK_DEP_CB],
[

CB_CFLAGS=""
CB_LIBS=""

requirements_pc=""
have_dep="yes"

dnl libarchive
PKG_CHECK_EXISTS([libarchive >= 3],
   [
    have_pkg_libarchive="yes"
    requirements_pc="libarchive >= 3 ${requirements_pc}"
   ],
   [have_pkg_libarchive="no"])

if test "x${have_pkg_libarchive}" = "xyes" ; then
   AC_DEFINE([HAVE_LIBARCHIVE], [1], [Set to 1 if libarchive is found])
fi

dnl check libraries
if ! test "x${requirements_pc}" = "x" ; then
   PKG_CHECK_MODULES([CB], [${requirements_pc}], [], [])
fi

if test "x$1" = "xstatic" ; then
   requirements_etui_pc="${requirements_pc} ${requirements_etui_pc}"
fi

AS_IF([test "x${have_dep}" = "xyes"], [$2], [$3])

])

dnl use: ETUI_CHECK_DEP_DJVU(want_static[, ACTION-IF-FOUND[, ACTION-IF-NOT-FOUND]])

AC_DEFUN([ETUI_CHECK_DEP_DJVU],
[

requirements_pc=""

dnl libdjvu
PKG_CHECK_EXISTS([ddjvuapi],
   [
    if test "x${enable_gpl}" = "xyes" ; then
       have_dep="yes"
       requirements_pc="ddjvuapi ${requirements_pc}"
    else
       have_no_gpl="yes"
       AC_MSG_WARN([pass --enable-gpl to enable Djvu module])
       have_dep="no"
    fi
   ],
   [have_dep="no"])

dnl check libraries
if test "x${have_dep}" = "xyes" ; then
   PKG_CHECK_MODULES([DJVU], [${requirements_pc}], [], [])
fi

if test "x$1" = "xstatic" ; then
   requirements_etui_pc="${requirements_pc} ${requirements_etui_pc}"
fi

AS_IF([test "x${have_dep}" = "xyes"], [$2], [$3])

])

dnl use: ETUI_CHECK_DEP_EPUB(want_static[, ACTION-IF-FOUND[, ACTION-IF-NOT-FOUND]])

AC_DEFUN([ETUI_CHECK_DEP_EPUB],
[

EPUB_CFLAGS=""
EPUB_LIBS=""

requirements_pc=""

dnl libarchive, ewebkit2, ecore_file
PKG_CHECK_EXISTS([libarchive >= 3 ewebkit2 >= ${efl_version} ecore-file >= ${efl_version}],
   [
    have_dep="yes"
    requirements_pc="libarchive >= 3 ewebkit2 >= ${efl_version} ecore-file >= ${efl_version} ${requirements_pc}"
   ],
   [have_dep="no"])

dnl check libraries
if ! test "x${requirements_pc}" = "x" ; then
   PKG_CHECK_MODULES([EPUB], [${requirements_pc}], [], [])
fi

if test "x$1" = "xstatic" ; then
   requirements_etui_pc="${requirements_pc} ${requirements_etui_pc}"
fi

AS_IF([test "x${have_dep}" = "xyes"], [$2], [$3])

])

dnl use: ETUI_CHECK_DEP_PDF(want_static[, ACTION-IF-FOUND[, ACTION-IF-NOT-FOUND]])

AC_DEFUN([ETUI_CHECK_DEP_PDF],
[

MUPDF_CFLAGS=""
MUPDF_LIBS=""

requirements_pc=""
requirements_libs=""

have_dep="yes"

dnl zlib
if test "x${have_dep}" = "xyes" ; then
   PKG_CHECK_EXISTS([zlib >= 1.2.5],
      [requirements_pc="zlib ${requirements_pc}"], [have_dep="no"])
fi

dnl jpeglib
if test "x${have_dep}" = "xyes" ; then
   AC_CHECK_HEADER([jpeglib.h], [have_dep="yes"], [have_dep="no"])
   if test "x${have_dep}" = "xyes" ; then
      AC_CHECK_LIB([jpeg], [jpeg_std_error],
         [requirements_libs="${requirements_libs} -ljpeg"],
         [have_dep="no"])
   fi
fi

dnl freetype
if test "x${have_dep}" = "xyes" ; then
   PKG_CHECK_EXISTS([freetype2],
      [requirements_pc="freetype2 ${requirements_pc}"],
      [have_dep="no"])
fi

dnl harfbuzz
if test "x${have_dep}" = "xyes" ; then
   PKG_CHECK_EXISTS([harfbuzz],
      [requirements_pc="harfbuzz ${requirements_pc}"],
      [have_dep="no"])
fi

dnl openssl
if test "x${have_dep}" = "xyes" ; then
   PKG_CHECK_EXISTS([openssl >= 1],
      [requirements_pc="openssl ${requirements_pc}"],
      [have_dep="no"])
fi

dnl check libraries

if test "x${have_dep}" = "xyes" ; then
   PKG_CHECK_MODULES([MUPDF],
      [${requirements_pc}],
      [
       have_dep="yes"
       MUPDF_LIBS="${requirements_libs} ${MUPDF_LIBS}"
      ],
      [have_dep="no"])
fi

AC_ARG_VAR([MUPDF_CFLAGS], [preprocessor flags for mupdf])
AC_SUBST([MUPDF_CFLAGS])
AC_ARG_VAR([MUPDF_LIBS], [linker flags for mupdf])
AC_SUBST([MUPDF_LIBS])

AC_SUBST([JPEG_LIBS])
AC_SUBST([OPENJPEG_LIBS])

if test "x$1" = "xstatic" ; then
   requirements_etui_pc="${requirements_pc} ${requirements_etui_pc}"
   requirements_etui_libs="${requirements_libs} ${requirements_etui_libs}"
fi

AS_IF([test "x${have_dep}" = "xyes"], [$2], [$3])

])

dnl use: ETUI_CHECK_DEP_PS(want_static[, ACTION-IF-FOUND[, ACTION-IF-NOT-FOUND]])

AC_DEFUN([ETUI_CHECK_DEP_PS],
[

PS_CFLAGS=""
PS_LIBS=""

requirements_pc=""

dnl xpost
PKG_CHECK_EXISTS([xpost],
   [
    have_dep="yes"
    requirements_pc="xpost ${requirements_pc}"
   ],
   [have_dep="no"])

dnl check libraries
if ! test "x${requirements_pc}" = "x" ; then
   PKG_CHECK_MODULES([XPOST], [${requirements_pc}], [], [])
fi

if test "x$1" = "xstatic" ; then
   requirements_etui_pc="${requirements_pc} ${requirements_etui_pc}"
fi

AS_IF([test "x${have_dep}" = "xyes"], [$2], [$3])

])

dnl use: ETUI_CHECK_DEP_TIFF(want_static[, ACTION-IF-FOUND[, ACTION-IF-NOT-FOUND]])

AC_DEFUN([ETUI_CHECK_DEP_TIFF],
[

TIFF_CFLAGS=""
TIFF_LIBS=""

requirements_pc=""

dnl libtiff
PKG_CHECK_EXISTS([libtiff-4],
   [
    have_dep="yes"
    requirements_pc="libtiff-4 ${requirements_pc}"
   ],
   [have_dep="no"])

dnl check libraries
if ! test "x${requirements_pc}" = "x" ; then
   PKG_CHECK_MODULES([TIFF], [${requirements_pc}], [], [])
fi

if test "x$1" = "xstatic" ; then
   requirements_etui_pc="${requirements_pc} ${requirements_etui_pc}"
fi

AS_IF([test "x${have_dep}" = "xyes"], [$2], [$3])

])

dnl use: ETUI_CHECK_DEP_TXT(want_static[, ACTION-IF-FOUND[, ACTION-IF-NOT-FOUND]])

AC_DEFUN([ETUI_CHECK_DEP_TXT],
[
have_dep="yes"

AS_IF([test "x${have_dep}" = "xyes"], [$2], [$3])

])

dnl use: ETUI_CHECK_MODULE(description, want_module[, ACTION-IF-FOUND[, ACTION-IF-NOT-FOUND]])
AC_DEFUN([ETUI_CHECK_MODULE],
[
m4_pushdef([UP], m4_translit([$1], [-a-z], [_A-Z]))dnl
m4_pushdef([DOWN], m4_translit([$1], [-A-Z], [_a-z]))dnl

want_module="$2"

AC_ARG_ENABLE([DOWN],
   [AS_HELP_STRING([--enable-]DOWN, [enable build of $1 module @<:@default=yes@:>@])],
   [
    if test "x${enableval}" = "xyes" ; then
       enable_module="strict"
    else
       if test "x${enableval}" = "xstatic" ; then
          enable_module="static"
       else
          enable_module="no"
       fi
    fi
   ],
   [enable_module="yes"])

if test "x${enable_module}" = "xyes" || test "x${enable_module}" = "xstrict" || test "x${enable_module}" = "xstatic" ; then
   want_module="yes"
fi

have_module="no"
if test "x${want_module}" = "xyes" && (test "x${enable_module}" = "xyes" || test "x${enable_module}" = "xstrict" || test "x${enable_module}" = "xstatic") ; then
   m4_default([ETUI_CHECK_DEP_]m4_defn([UP]))(${enable_module}, [have_module="yes"], [have_module="no"])
fi

AC_MSG_CHECKING([whether to enable $1 module built])
AC_MSG_RESULT([${have_module}])

if test "x${have_module}" = "xno" && test "x${enable_module}" = "xstrict" ; then
   AC_MSG_ERROR([$1 module requested, but dependency checks failed. See config.log for more details. Exiting...])
fi

static_module="no"
if test "x${have_module}" = "xyes" && test "x${enable_module}" = "xstatic" ; then
   static_module="yes"
fi

AM_CONDITIONAL(ETUI_BUILD_[]UP, [test "x${have_module}" = "xyes"])
AM_CONDITIONAL(ETUI_BUILD_STATIC_[]UP, [test "x${static_module}" = "xyes"])

if test "x${static_module}" = "xyes" ; then
   AC_DEFINE(ETUI_BUILD_STATIC_[]UP, [1], [Set to 1 if $1 is statically built])
   have_static_module="yes"
fi

enable_[]DOWN="no"
if test "x${have_module}" = "xyes" ; then
   enable_[]DOWN=${enable_module}
   AC_DEFINE(ETUI_BUILD_[]UP, [1], [Set to 1 if $1 is built])
fi

AS_IF([test "x$have_module" = "xyes"], [$3], [$4])

m4_popdef([UP])
m4_popdef([DOWN])
])
