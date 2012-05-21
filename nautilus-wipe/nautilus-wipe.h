/*
 *  nautilus-wipe - a nautilus extension to wipe file(s)
 * 
 *  Copyright (C) 2009-2011 Colomban Wendling <ban@herbesfolles.org>
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

#ifndef NAUTILUS_WIPE_H
#define NAUTILUS_WIPE_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS


#define NAUTILUS_TYPE_WIPE  (nautilus_wipe_get_type ())
#define NAUTILUS_WIPE(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), NAUTILUS_TYPE_WIPE, NautilusWipe))
#define NAUTILUS_IS_WIPE(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), NAUTILUS_TYPE_WIPE))
typedef struct _NautilusWipe      NautilusWipe;
typedef struct _NautilusWipeClass NautilusWipeClass;

#define NAUTILUS_WIPE_ERROR (nautilus_wipe_error_quark ())

typedef enum {
  NAUTILUS_WIPE_ERROR_SPAWN_FAILED,
  NAUTILUS_WIPE_ERROR_CHILD_CRASHED,
  NAUTILUS_WIPE_ERROR_CHILD_FAILED,
  NAUTILUS_WIPE_ERROR_UNSUPPORTED_LOCATION,
  NAUTILUS_WIPE_ERROR_NOT_IMPLEMENTED,
  NAUTILUS_WIPE_ERROR_FAILED
} NautilusWipeError;

struct _NautilusWipe {
  GObject parent_slot;
};

struct _NautilusWipeClass {
  GObjectClass parent_slot;
};

GType   nautilus_wipe_get_type        (void) G_GNUC_CONST;
GType   nautilus_wipe_register_type   (GTypeModule *module);
GQuark  nautilus_wipe_error_quark     (void) G_GNUC_CONST;


G_END_DECLS

#endif /* guard */
