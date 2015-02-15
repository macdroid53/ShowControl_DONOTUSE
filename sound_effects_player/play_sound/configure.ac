dnl Process this file with autoconf to produce a configure script.
dnl Created originally by the Anjuta application wizard.

AC_INIT(play_sound, 0.16, [John_Sauter@systemeyescomputerstore.com], play_sound)
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([1.11])
AM_SILENT_RULES([yes])
AM_MAINTAINER_MODE([enable])
AC_CONFIG_MACRO_DIR([m4])
AC_PROG_CC
AC_CHECK_LIB([m], [pow])

dnl ***************************************************************************
dnl Internationalization
dnl ***************************************************************************
IT_PROG_INTLTOOL([0.35.0])

GETTEXT_PACKAGE=play_sound
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [GETTEXT package name])

AM_GLIB_GNU_GETTEXT





LT_INIT


PKG_CHECK_MODULES(PLAY_SOUND, [gtk+-3.0 >= 3.14 gstreamer-1.0 gtk+-3.0 gstreamer-allocators-1.0 gstreamer-plugins-base-1.0 gstreamer-base-1.0 gstreamer-app-1.0 gstreamer-controller-1.0 gstreamer-audio-1.0])




AC_OUTPUT([
Makefile
src/Makefile
po/Makefile.in
])
