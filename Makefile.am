SUBDIRS = nautilus-srm po help

ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = m4/na-nautilus-extdir.m4 \
             build-aux/config.rpath \
             build-aux/gnome-doc-utils.make \
             po/Makevars.in
DISTCLEANFILES = po/Makevars \
                 build-aux/gnome-doc-utils.make
DISTCHECK_CONFIGURE_FLAGS = --disable-scrollkeeper
