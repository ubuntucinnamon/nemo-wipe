nautilus_extensiondir = $(NAUTILUS_EXTENSIONS_DIR)

nautilus_extension_LTLIBRARIES = libnautilus-wipe.la

localedir = $(datadir)/locale

libnautilus_wipe_la_SOURCES  = extension.c \
                               nw-compat.h \
                               nw-delete-operation.c \
                               nw-delete-operation.h \
                               nw-extension.c \
                               nw-extension.h \
                               nw-fill-operation.c \
                               nw-fill-operation.h \
                               nw-operation.c \
                               nw-operation.h \
                               nw-operation-manager.c \
                               nw-operation-manager.h \
                               nw-path-list.c \
                               nw-path-list.h \
                               nw-progress-dialog.c \
                               nw-progress-dialog.h \
                               nw-type-utils.h
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
