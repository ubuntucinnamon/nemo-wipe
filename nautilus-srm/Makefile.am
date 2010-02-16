nautilus_extensiondir = $(NAUTILUS_EXTENSIONS_DIR)

nautilus_extension_LTLIBRARIES = libnautilus-srm.la

libnautilus_srm_la_SOURCES = nautilus-srm.c \
                             delete-operation.c \
                             fill-operation.c \
                             nautilus-srm.h

libnautilus_srm_la_LDFLAGS = $(AM_LIBS) -module -avoid-version
