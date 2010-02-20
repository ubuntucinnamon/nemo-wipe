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

#include <gsecuredelete/gsecuredelete.h>
#include <libnautilus-extension/nautilus-file-info.h>


/*
 * nsrm_delete_operation:
 * @files: A list of #NautilusFileInfo to delete.
 * @finished_handler: A handler for GsdAsyncOperation::finished
 * @progress_handler: A handler for GsdAsyncOperation::progress
 * @data: User data to pass to @finished_handler and @progress_handler
 * @error: Return location for errors or %NULL to ignore them. The errors are
 *         those from gsd_secure_delete_operation_run().
 * 
 * Deletes the given files with libgsecuredelete.
 * 
 * Returns: %TRUE if operation successfully started, %FALSE otherwise. %TRUE
 *          does not mean that anything was actually done, but only that the
 *          operation started successfully.
 */
gboolean
nautilus_srm_delete_operation (GList    *files,
                               GCallback finished_handler,
                               GCallback progress_handler,
                               gpointer  data,
                               GError  **error)
{
  gboolean            success = TRUE;
  GsdDeleteOperation *operation;
  
  operation = gsd_delete_operation_new ();
  for (; success && files; files = g_list_next (files)) {
    GFile *file = nautilus_file_info_get_location (files->data);
    gchar *path;
    
    path = g_file_get_path (file);
    if (! path) {
      gchar *uri = g_file_get_uri (file);
      
      success = FALSE;
      /* FIXME: use correct error quark and code */
      g_set_error (error, 0, 0, "Unsupported location: %s", uri);
      g_free (uri);
    } else {
      gsd_delete_operation_add_path (operation, path);
    }
    g_free (path);
    g_object_unref (file);
  }
  /* if file addition succeeded, try to launch operation */
  if (success) {
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
  
  return success;
}

