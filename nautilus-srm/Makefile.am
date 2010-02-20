nautilus_extensiondir = $(NAUTILUS_EXTENSIONS_DIR)

nautilus_extension_LTLIBRARIES = libnautilus-srm.la

libnautilus_srm_la_SOURCES = nautilus-srm.c \
                             nautilus-srm.h \
                             operation-manager.c \
                             operation-manager.h \
                             delete-operation.c \
                             delete-operation.h \
                             fill-operation.c \
                             fill-operation.h \
                             progress-dialog.c \
                             progress-dialog.h

libnautilus_srm_la_LDFLAGS = $(AM_LIBS) -module -avoid-version
