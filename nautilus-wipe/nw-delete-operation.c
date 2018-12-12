/*
 *  nautilus-wipe - a nautilus extension to wipe file(s)
 * 
 *  Copyright (C) 2009-2012 Colomban Wendling <ban@herbesfolles.org>
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "nw-delete-operation.h"

#include <glib.h>
#include <glib/gi18n-lib.h>
#include <glib-object.h>
#include <gsecuredelete.h>

#include "nw-operation.h"


static void   nw_delete_operation_opeartion_iface_init    (NwOperationInterface *iface);
static void   nw_delete_operation_real_add_file           (NwOperation *self,
                                                           const gchar *file);
static gchar *nw_delete_operation_real_get_progress_step  (NwOperation *self);


G_DEFINE_TYPE_WITH_CODE (NwDeleteOperation,
                         nw_delete_operation,
                         GSD_TYPE_DELETE_OPERATION,
                         G_IMPLEMENT_INTERFACE (NW_TYPE_OPERATION,
                                                nw_delete_operation_opeartion_iface_init))


static void
nw_delete_operation_opeartion_iface_init (NwOperationInterface *iface)
{
  iface->add_file           = nw_delete_operation_real_add_file;
  iface->get_progress_step  = nw_delete_operation_real_get_progress_step;
}

static void
nw_delete_operation_class_init (NwDeleteOperationClass *klass)
{
}

static void
nw_delete_operation_init (NwDeleteOperation *self)
{
}

static void
nw_delete_operation_real_add_file (NwOperation *self,
                                   const gchar *file)
{
  gsd_delete_operation_add_path (GSD_DELETE_OPERATION (self), file);
}

static gchar *
nw_delete_operation_real_get_progress_step (NwOperation *operation)
{
  /* it's a bit ugly here: we rely on the knowledge that
   * GsdSecureDeleteOperation's get_max_progress() returns the individual
   * pass count, and GsdDeleteOperation overrides it multiplying it by the
   * file count.  But well, that gives us everything but the file name. */
  GsdAsyncOperation      *op        = GSD_ASYNC_OPERATION (operation);
  GsdAsyncOperationClass *delopcls  = g_type_class_peek (GSD_TYPE_SECURE_DELETE_OPERATION);
  guint                   passes    = delopcls->get_max_progress (op);
  guint                   n_files   = op->n_passes / passes;
  guint                   file      = op->passes / passes;
  guint                   pass      = op->passes % passes;
  
  return g_strdup_printf (_("File %u out of %u, pass %u out of %u"),
                          file + 1, n_files, pass + 1, passes);
}

NwOperation *
nw_delete_operation_new (void)
{
  return g_object_new (NW_TYPE_DELETE_OPERATION, NULL);
}
