nautilus_extensiondir = $(NAUTILUS_EXTENSIONS_DIR)

nautilus_extension_LTLIBRARIES = libnautilus-srm.la

noinst_PROGRAMS = test

localedir = $(datadir)/locale

libnautilus_srm_la_SOURCES = nautilus-srm.c \
                             nautilus-srm.h \
                             operation-manager.c \
                             operation-manager.h \
                             delete-operation.c \
                             delete-operation.h \
                             fill-operation.c \
                             fill-operation.h \
                             progress-dialog.c \
                             progress-dialog.h \
                             compat.h
libnautilus_srm_la_CPPFLAGS = -DG_LOG_DOMAIN=\"$(PACKAGE)\" \
                              -DLOCALEDIR=\"$(localedir)\"
libnautilus_srm_la_CFLAGS   = @GLIB_CFLAGS@ \
                              @GTK_CFLAGS@ \
                              @GCONF_CFLAGS@ \
                              @LIBNAUTILUS_EXTENSION_CFLAGS@ \
                              @GSECUREDELETE_CFLAGS@ \
                              @GIO_UNIX_CFLAGS@
libnautilus_srm_la_LDFLAGS  = -module -avoid-version \
                              @GLIB_LIBS@ \
                              @GTK_LIBS@ \
                              @GCONF_LIBS@ \
                              @LIBNAUTILUS_EXTENSION_LIBS@ \
                              @GSECUREDELETE_LIBS@ \
                              @GIO_UNIX_LIBS@

test_CFLAGS  = @GLIB_CFLAGS@ \
               @GTK_CFLAGS@
test_LDFLAGS = @GLIB_LIBS@ \
               @GTK_LIBS@
# Linking against a loadable module is not portable according to libtool.
# However, I don't really care about portability issue here because it is only a
# test program (that might even be removed at a time).
test_LDADD   = libnautilus-srm.la
test_SOURCES = test.c
