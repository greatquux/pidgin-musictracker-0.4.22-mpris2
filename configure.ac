# Copyright (C) 2006 Arijit De <de.arijit@gmail.com>
#  
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without 
# modifications, as long as this notice is preserved.
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

AC_INIT([MusicTracker],
        [m4_esyscmd([cat version | tr -d '\n'])],
        [pidgin-musictracker@googlegroups.com],
        [pidgin-musictracker])
AC_CONFIG_AUX_DIR(config)
AM_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE([dist-bzip2])

AC_PROG_CC
AC_PROG_LIBTOOL
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION(0.17)

AC_ARG_ENABLE(debug,
[  --enable-debug          compile with debugging support],,enable_debug=no)
if test "x$enable_debug" = "xyes" ; then
	DEBUG_CFLAGS="-g -O0"
fi
AC_SUBST(DEBUG_CFLAGS)

PKG_CHECK_MODULES([PIDGIN],[pidgin >= 2.0.0],,AC_MSG_ERROR([
*** Pidgin 2.0+ is required to build pidgin-musictracker
*** Please make sure you have the Pidgin development files installed.
*** The latest version of Pidgin is always available at http://pidgin.im/]))
AC_SUBST(PIDGIN_CFLAGS)
PIDGIN_LIBDIR=`pkg-config --variable=libdir pidgin`
PIDGIN_DATADIR=`pkg-config --variable=datadir pidgin`
AC_SUBST(PIDGIN_LIBDIR)
AC_SUBST(PIDGIN_DATADIR)

AM_PATH_GLIB_2_0(2.0.0,,AC_MSG_ERROR([
*** GLib 2.0+ is required to build pidgin-musictracker
*** Please make sure you have the GLib development files installed
*** The latest version of GLib is always available at http://www.gtk.org/]))
AC_SUBST(GLIB_LIBS)
AC_SUBST(GLIB_CFLAGS)

AM_PATH_GTK_2_0(2.0.0,,AC_MSG_ERROR([
*** GTK+ 2.0+ is required to build pidgin-musictracker
*** Please make sure you have the GTK+ development files installed
*** The latest version of GTK+ is always available at http://www.gtk.org/]))
AC_SUBST(GTK_LIBS)
AC_SUBST(GTK_CFLAGS)

PKG_CHECK_MODULES([DBUS],[dbus-glib-1 >= 0.73],,AC_MSG_ERROR([
*** DBUS 0.73+ is required to build pidgin-musictracker
*** Please make sure you have the DBUS development files installed
*** The latest version of DBUS is always available at http://dbus.freedesktop.net/]))
AC_SUBST(DBUS_CFLAGS)
AC_SUBST(DBUS_LIBS)

AC_ARG_ENABLE(xmms2,
[  --enable-xmms2          Enable XMMS2 support (default=auto)],
        enable_xmms2=$enableval, enable_xmms2=auto)

if test "x$enable_xmms2" = "xauto" ; then
	PKG_CHECK_MODULES([XMMSCLIENT], [xmms2-client >= 0.6],enable_xmms2=yes,enable_xmms2=no)
fi

if test "x$enable_xmms2" = "xyes" ; then
	PKG_CHECK_MODULES([XMMSCLIENT], [xmms2-client >= 0.6],[AC_DEFINE(HAVE_XMMSCLIENT, 1, "XMMS2 Client")],AC_MSG_ERROR([
	*** XMMS2 0.6+ is required to build pidgin-musictracker with XMMS2 support
        *** Please make sure you have the XMMS2 development files installed
        *** The latest version of XMMS2 is always available at http://wiki.xmms2.xmms.se/]))
	AC_SUBST(XMMSCLIENT_CFLAGS)
	AC_SUBST(XMMSCLIENT_LIBS)
fi

# We want warnings to be treated as errors,
# unless the --disable-werror configure option was used.
AC_ARG_ENABLE(werror,
 AC_HELP_STRING([--disable-werror], [do not use -Werror compiler flag (default=yes)]))

WERROR_CFLAGS=""
if test x"$enable_werror" != xno
then
        WERROR_CFLAGS="-Werror"
fi

AC_SUBST(WERROR_CFLAGS)

PKG_CHECK_MODULES([PCRE],[libpcre],,AC_MSG_ERROR([
*** PCRE library is required to build pidgin-musictracker
*** Please make sure you have the PCRE development files installed
*** The latest version of PCRE is always available at http://www.pcre.org/]))
AC_SUBST(PCRE_LIBS)
AC_SUBST(PCRE_CFLAGS)

AC_PREFIX_DEFAULT(`pkg-config --variable=prefix pidgin`)

AC_CONFIG_FILES([ po/Makefile.in
   Makefile
   README
   doc/Makefile
   m4/Makefile
   src/Makefile
   installer/Makefile
])

AC_OUTPUT
