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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "delete-operation.h"

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>
#include <gsecuredelete/gsecuredelete.h>


/*
 * nsrm_delete_operation:
 * @files: A list of paths to delete.
 * @fast: The Gsd.SecureDeleteOperation:fast setting
 * @mode: The Gsd.SecureDeleteOperation:mode setting
 * @zeroise: The Gsd.ZeroableOperation:zeroise setting
 * @finished_handler: A handler for GsdAsyncOperation::finished
 * @progress_handler: A handler for GsdAsyncOperation::progress
 * @data: User data to pass to @finished_handler and @progress_handler
 * @error: Return location for errors or %NULL to ignore them. The errors are
 *         those from gsd_secure_delete_operation_run().
 * 
 * Deletes the given files with libgsecuredelete.
 * 
 * Returns: The operation object that was launched, or %NULL on failure.
 *          The operation object should be unref'd with g_object_unref() when
 *          no longer needed.
 */
GsdAsyncOperation *
nautilus_srm_delete_operation (GList                       *files,
                               gboolean                     fast,
                               GsdSecureDeleteOperationMode mode,
                               gboolean                     zeroise,
                               GCallback                    finished_handler,
                               GCallback                    progress_handler,
                               gpointer                     data,
                               GError                     **error)
{
  gboolean            success = TRUE;
  GsdDeleteOperation *operation;
  guint               n_files = 0;
  
  operation = gsd_delete_operation_new ();
  for (; files; files = g_list_next (files)) {
    gsd_delete_operation_add_path (operation, files->data);
    n_files ++;
  }
  if (n_files < 1) {
    /* FIXME: use correct error quark and code */
    g_set_error (error, 0, 0, "Nothing to do!");
    success = FALSE;
  } else {
    /* if file addition succeeded, try to launch operation */
    gsd_secure_delete_operation_set_fast (GSD_SECURE_DELETE_OPERATION (operation), fast);
    gsd_secure_delete_operation_set_mode (GSD_SECURE_DELETE_OPERATION (operation), mode);
    gsd_zeroable_operation_set_zeroise (GSD_ZEROABLE_OPERATION (operation), zeroise);
    g_signal_connect (operation, "progress", progress_handler, data);
    g_signal_connect (operation, "finished", finished_handler, data);
    /* unrefs the operation when done (notice that it is called after the default
     * handler) */
    g_signal_connect_after (operation, "finished",
                            G_CALLBACK (g_object_unref), NULL);
    success = gsd_secure_delete_operation_run (GSD_SECURE_DELETE_OPERATION (operation),
                                               100, error);
  }
  /* if something failed, abort */
  if (! success) {
    /* on failure here the callback will not be called, then unref right here */
    g_object_unref (operation);
  }
  
  return success ? g_object_ref (operation) : NULL;
}

