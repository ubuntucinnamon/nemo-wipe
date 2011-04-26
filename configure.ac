# Initially based on NautilusActions configure & build system.

AC_PREREQ([2.64])
AC_INIT([nautilus-srm],[0.1])
AC_CONFIG_SRCDIR([nautilus-srm/nautilus-srm.h])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([1.11.1 -Wall -Werror foreign])
AC_CONFIG_HEADERS([config.h])

AM_SILENT_RULES([yes])

# Checks for programs.
LT_PREREQ([2.2.6])
LT_INIT([disable-static])
AC_PROG_CC
AM_PROG_CC_C_O
IT_PROG_INTLTOOL

GNOME_DOC_INIT

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
GIO_UNIX_REQUIRED=2.0

# mandatory packages
PKG_CHECK_MODULES([GLIB],                   glib-2.0              >= ${GLIB_REQUIRED})
PKG_CHECK_MODULES([GTK],                    gtk+-2.0              >= ${GTK_REQUIRED})
PKG_CHECK_MODULES([GCONF],                  gconf-2.0             >= ${GCONF_REQUIRED})
PKG_CHECK_MODULES([LIBNAUTILUS_EXTENSION],  libnautilus-extension >= ${NAUTILUS_EXTENSION})
PKG_CHECK_MODULES([GSECUREDELETE],          gsecuredelete         >= ${GSECUREDELETE_REQUIRED})

# optional packages
HAVE_GIO_UNIX=0
PKG_CHECK_MODULES([GIO_UNIX], gio-unix-2.0 >= ${GIO_UNIX_REQUIRED},
                  [ HAVE_GIO_UNIX=1
                    AC_SUBST([GIO_UNIX_CFLAGS])
                    AC_SUBST([GIO_UNIX_LIBS]) ],
                  [ AC_MSG_WARN([GIO-UNIX not found. Some UNIX mount points might not be detected correctly. The error was: ${GIO_UNIX_PKG_ERRORS}])])
AC_DEFINE_UNQUOTED([HAVE_GIO_UNIX], [${HAVE_GIO_UNIX}], [Whether we have gio-unix])

# i18n
GETTEXT_PACKAGE="${PACKAGE}"
AC_SUBST([GETTEXT_PACKAGE])
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["${GETTEXT_PACKAGE}"], [GNU gettext package])

AM_GLIB_GNU_GETTEXT

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# when working in a test environment, nautilus is typically installed in
# a non-standard location ; lets specify this here
NA_NAUTILUS_EXTDIR

# Checks for library functions.
# If we are here we're sure we have libnautilus-extension, but we need this for
# the check below to work (for the lib to be found).
AC_CHECK_LIB([nautilus-extension], [nautilus_menu_provider_get_file_items])
AC_CHECK_FUNCS([nautilus_file_info_get_location])

# Output
AC_CONFIG_FILES([Makefile
                 po/Makefile.in
                 nautilus-srm/Makefile
                 help/Makefile])
AC_OUTPUT
