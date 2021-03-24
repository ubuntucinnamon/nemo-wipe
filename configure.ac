AC_CONFIG_HEADERS([config.h])

YELP_HELP_INIT

GTK_REQUIRED=2.6
GLIB_REQUIRED=2.10
GOBJECT_REQUIRED=2.4.0
GCONF_REQUIRED=2.0
NEMO_EXTENSION=2.0
GSECUREDELETE_REQUIRED=0.3
GIO_UNIX_REQUIRED=2.0

# mandatory packages
PKG_CHECK_MODULES([GLIB],                   glib-2.0                      >= ${GLIB_REQUIRED})
PKG_CHECK_MODULES([GTK],                    $gtk_pkg                      >= ${GTK_REQUIRED})
PKG_CHECK_MODULES([LIBNEMO_EXTENSION],  lib${with_nemo}-extension >= ${NEMO_EXTENSION})
PKG_CHECK_MODULES([GSECUREDELETE],          gsecuredelete                 >= ${GSECUREDELETE_REQUIRED})

# i18n
GETTEXT_PACKAGE="${PACKAGE}"
AC_SUBST([GETTEXT_PACKAGE])
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["${GETTEXT_PACKAGE}"], [GNU gettext package])

# when working in a test environment, nemo is typically installed in
# a non-standard location ; lets specify this here
NW_NEMO_EXTDIR([$with_nemo])

# Output
AC_CONFIG_FILES([Makefile
                 po/Makefile.in
                 nemo-wipe/Makefile
                 help/Makefile])
