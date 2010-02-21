/*
 *  nautilus-srm - a nautilus extension to wipe file(s) with srm
 * 
 *  Copyright (C) 2009-2010 Colomban Wendling <ban@herbesfolles.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 3 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef NAUTILUS_SRM_H
#define NAUTILUS_SRM_H

#include <glib.h>
#include <glib-object.h>
#define GETTEXT_PACKAGE "nautilus-srm"
#include <glib/gi18n-lib.h>

/* if GLib doesn't provide g_dngettext(), wrap it from dngettext() */
#if (! GLIB_CHECK_VERSION (2, 18, 0) && ! defined (g_dngettext))
# include <libintl.h>
# define g_dngettext dngettext
#endif

G_BEGIN_DECLS

/* Declarations for the open terminal extension object.  This object will be
 * instantiated by nautilus.  It implements the GInterfaces 
 * exported by libnautilus. */


#define NAUTILUS_TYPE_SRM   (nautilus_srm_get_type ())
#define NAUTILUS_SRM(o)     (G_TYPE_CHECK_INSTANCE_CAST ((o), NAUTILUS_TYPE_SRM, NautilusSrm))
#define NAUTILUS_IS_SRM(o)  (G_TYPE_CHECK_INSTANCE_TYPE ((o), NAUTILUS_TYPE_SRM))
typedef struct _NautilusSrm       NautilusSrm;
typedef struct _NautilusSrmClass  NautilusSrmClass;

#define NAUTILUS_SRM_ERROR (nautilus_srm_error_quark ())

typedef enum {
  NAUTILUS_SRM_ERROR_SPAWN_FAILED,
  NAUTILUS_SRM_ERROR_CHILD_CRASHED,
  NAUTILUS_SRM_ERROR_CHILD_FAILED,
  NAUTILUS_SRM_ERROR_UNSUPPORTED_LOCATION,
  NAUTILUS_SRM_ERROR_NOT_IMPLEMENTED,
  NAUTILUS_SRM_ERROR_FAILED
} NautilusSrmError;

struct _NautilusSrm {
  GObject parent_slot;
};

struct _NautilusSrmClass {
  GObjectClass parent_slot;
};

GType   nautilus_srm_get_type      (void) G_GNUC_CONST;
GQuark  nautilus_srm_error_quark   (void) G_GNUC_CONST;

G_END_DECLS

#endif /* guard */
