# Based on NautilusActions configure & build system, some things copied from it.


AC_PREREQ(2.64)
AC_INIT([nautilus-srm],[0.1])
AC_CONFIG_SRCDIR([nautilus-srm/nautilus-srm.h])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([-Wall -Werror])
AC_CONFIG_HEADERS([config.h])

m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

# Checks for programs.
AC_PROG_CC
# ?
AM_DISABLE_STATIC
AM_PROG_LIBTOOL

# Checks for libraries.
# 2.6 for gtk_message_dialog_format_secondary_text()
# We use some newer functions, but we (should) only do so if available.
GTK_REQUIRED=2.6
# 2.10 to have the slice allocator (may be dropped if needed, but its probably a
# acceptable dependency).
# We use g_dngettext() (a 2.18 feature), but only conditionally if present.
GLIB_REQUIRED=2.10
# 2.4 for g_type_class_add_private()
GOBJECT_REQUIRED=2.4.0
GCONF_REQUIRED=2.0
NAUTILUS_EXTENSION=2.0
GSECUREDELETE_REQUIRED=0.1

PKG_CHECK_MODULES([NAUTILUS_SRM], \
  glib-2.0              >= ${GLIB_REQUIRED}           \
  gtk+-2.0              >= ${GTK_REQUIRED}            \
  gconf-2.0             >= ${GCONF_REQUIRED}          \
  libnautilus-extension >= ${NAUTILUS_EXTENSION}      \
  gsecuredelete         >= ${GSECUREDELETE_REQUIRED}  \
)

GIO_UNIX_REQUIRED=2.0

HAVE_GIO_UNIX=0
PKG_CHECK_MODULES([GIO_UNIX], gio-unix-2.0 >= ${GIO_UNIX_REQUIRED},
                  [ HAVE_GIO_UNIX=1
                    AC_SUBST([GIO_UNIX_CFLAGS])
                    AC_SUBST([GIO_UNIX_LIBS]) ],
                  [ echo "	$GIO_UNIX_PKG_ERRORS">&2 ])
AC_DEFINE_UNQUOTED([HAVE_GIO_UNIX], [${HAVE_GIO_UNIX}], [Whether we have gio-unix])

AC_SUBST([NAUTILUS_SRM_CFLAGS])
AC_SUBST([NAUTILUS_SRM_LIBS])
AC_SUBST([AM_CFLAGS],["${AM_CFLAGS} ${NAUTILUS_SRM_CFLAGS} ${GIO_UNIX_CFLAGS}"])
AC_SUBST([AM_LIBS],["${AM_LIBS} ${NAUTILUS_SRM_LIBS} ${GIO_UNIX_LIBS}"])

# i18n
AM_GNU_GETTEXT([external])

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE

# Log domain
AC_SUBST([AM_CPPFLAGS],["${AM_CPPFLAGS} -DG_LOG_DOMAIN=\\\"${PACKAGE}\\\""])

# when working in a test environment, nautilus is typically installed in
# a non-standard location ; lets specify this here
NA_NAUTILUS_EXTDIR

# Checks for library functions.
# If we are here we're sure we have libnautilus-extension, but we need this for
# the check below to work (for the lib to be found).
AC_CHECK_LIB([nautilus-extension], [nautilus_menu_provider_get_file_items])
AC_CHECK_FUNCS([nautilus_file_info_get_location])

# Output
AC_CONFIG_FILES([Makefile po/Makefile.in
                 nautilus-srm/Makefile])
AC_OUTPUT
