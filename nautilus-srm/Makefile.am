nautilus_extensiondir = $(NAUTILUS_EXTENSIONS_DIR)

nautilus_extension_LTLIBRARIES = libnautilus-srm.la

noinst_PROGRAMS = test

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

libnautilus_srm_la_LDFLAGS = $(AM_LIBS) -module -avoid-version

test_LDFLAGS = $(AM_LIBS)
# Linking against a loadable module is not portable according to libtool.
# However, I don't really care about portability issue here because it is only a
# test program (that might even be removed at a time).
test_LDADD   = libnautilus-srm.la
test_SOURCES = test.c
