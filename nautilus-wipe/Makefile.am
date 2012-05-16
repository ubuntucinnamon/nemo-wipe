nautilus_extensiondir = $(NAUTILUS_EXTENSIONS_DIR)

nautilus_extension_LTLIBRARIES = libnautilus-wipe.la

localedir = $(datadir)/locale

libnautilus_wipe_la_SOURCES  = nautilus-wipe.c \
                               nautilus-wipe.h \
                               operation-manager.c \
                               operation-manager.h \
                               delete-operation.c \
                               delete-operation.h \
                               fill-operation.c \
                               fill-operation.h \
                               progress-dialog.c \
                               progress-dialog.h \
                               compat.h
libnautilus_wipe_la_CPPFLAGS = -DG_LOG_DOMAIN=\"$(PACKAGE)\" \
                               -DLOCALEDIR=\"$(localedir)\"
libnautilus_wipe_la_CFLAGS   = @GLIB_CFLAGS@ \
                               @GTK_CFLAGS@ \
                               @GCONF_CFLAGS@ \
                               @LIBNAUTILUS_EXTENSION_CFLAGS@ \
                               @GSECUREDELETE_CFLAGS@ \
                               @GIO_UNIX_CFLAGS@
libnautilus_wipe_la_LDFLAGS  = -module -avoid-version \
                               @GLIB_LIBS@ \
                               @GTK_LIBS@ \
                               @GCONF_LIBS@ \
                               @LIBNAUTILUS_EXTENSION_LIBS@ \
                               @GSECUREDELETE_LIBS@ \
                               @GIO_UNIX_LIBS@
