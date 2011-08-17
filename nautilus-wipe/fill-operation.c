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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "fill-operation.h"

#include <glib.h>
#include <glib/gi18n-lib.h>
#include <glib-object.h>
#include <gio/gio.h>
#if HAVE_GIO_UNIX
# include <gio/gunixmounts.h>
#endif
#include <gsecuredelete/gsecuredelete.h>
#include "nautilus-wipe.h"


GQuark
nautilus_wipe_fill_operation_error_quark (void)
{
  static GQuark q = 0;
  
  if (G_UNLIKELY (q == 0)) {
    q = g_quark_from_static_string ("NautilusWipeFillOperationError");
  }
  
  return q;
}

#if HAVE_GIO_UNIX
/*
 * find_mountpoint_unix:
 * @path: An absolute path for which find the mountpoint.
 * 
 * Gets the UNIX mountpoint path of a given path.
 * <note>
 *  This function would actually never return %NULL since the mount point of
 *  every files is at least /.
 * </note>
 * 
 * Returns: The path of the mountpoint of @path or %NULL if not found.
 *          Free with g_free().
 */
static gchar *
find_mountpoint_unix (const gchar *path)
{
  gchar    *mountpoint = g_strdup (path);
  gboolean  found = FALSE;
  
  while (! found && mountpoint) {
    GUnixMountEntry *unix_mount;
    
    unix_mount = g_unix_mount_at (mountpoint, NULL);
    if (unix_mount) {
      found = TRUE;
      g_unix_mount_free (unix_mount);
    } else {
      gchar *tmp = mountpoint;
      
      mountpoint = g_path_get_dirname (tmp);
      /* check if dirname() changed the path to avoid infinite loop (e.g. when
       * / was reached) */
      if (strcmp (mountpoint, tmp) == 0) {
        g_free (mountpoint);
        mountpoint = NULL;
      }
      g_free (tmp);
    }
  }
  
  return mountpoint;
}
#endif

static gchar *
find_mountpoint (const gchar *path,
                 GError     **error)
{
  gchar  *mountpoint_path = NULL;
  GFile  *file;
  GMount *mount;
  GError *err = NULL;
  
  /* Try with GIO first */
  file = g_file_new_for_path (path);
  mount = g_file_find_enclosing_mount (file, NULL, &err);
  if (mount) {
    GFile *mountpoint_file;
    
    mountpoint_file = g_mount_get_root (mount);
    mountpoint_path = g_file_get_path (mountpoint_file);
    if (! mountpoint_path) {
      gchar *uri = g_file_get_uri (mountpoint_file);
      
      g_set_error (&err,
                   NAUTILUS_WIPE_FILL_OPERATION_ERROR,
                   NAUTILUS_WIPE_FILL_OPERATION_ERROR_REMOTE_MOUNT,
                   _("Mount \"%s\" is not local"), uri);
      g_free (uri);
    }
    g_object_unref (mountpoint_file);
    g_object_unref (mount);
  }
  g_object_unref (file);
  #if HAVE_GIO_UNIX
  /* fallback to find_unix_mount() */
  if (! mountpoint_path) {
    g_clear_error (&err);
    mountpoint_path = find_mountpoint_unix (path);
    if (! mountpoint_path) {
      g_set_error (&err,
                   NAUTILUS_WIPE_FILL_OPERATION_ERROR,
                   NAUTILUS_WIPE_FILL_OPERATION_ERROR_MISSING_MOUNT,
                   _("No mount point found for path \"%s\""), path);
    }
  }
  #endif
  if (! mountpoint_path) {
    g_propagate_error (error, err);
  }
  
  return mountpoint_path;
}




typedef void  (*FillFinishedFunc) (GsdFillOperation  *self,
                                   gboolean           success,
                                   const gchar       *message,
                                   gpointer           data);
typedef void  (*FillProgressFunc) (GsdFillOperation  *self,
                                   gdouble            fraction,
                                   gpointer           data);
/* The data structure that holds the operation state and data */
struct FillOperationData
{
  GsdFillOperation *operation;
  /* the current directory to work on (dir->data) */
  GList *dir;
  /* GObject's signals handlers IDs */
  gulong finished_hid;
  gulong progress_hid;
  /* operation status */
  guint n_op;
  guint n_op_done;
  
  /* caller's handlers for wrapping */
  FillFinishedFunc  finished_handler;
  FillProgressFunc  progress_handler;
  gpointer          cbdata;
};

static void   nautilus_wipe_fill_finished_handler (GsdFillOperation         *operation,
                                                   gboolean                  success,
                                                   const gchar              *message,
                                                   struct FillOperationData *opdata);
static void   nautilus_wipe_fill_progress_handler (GsdFillOperation         *operation,
                                                   gdouble                   fraction,
                                                   struct FillOperationData *opdata);


/* Actually calls libgsecuredelete */
static gboolean
do_fill_operation (struct FillOperationData *opdata,
                   GError                  **error)
{
  g_message ("Starting work on %s", (const gchar *)opdata->dir->data);
  /* FIXME: don't launch sfill in a useful directory since it can leave a bunch
   * of files if it get interrupted (e.g. from a crash, a user kill or so) */
  return gsd_fill_operation_run (opdata->operation, opdata->dir->data, error);
}

/* Removes the current directory to proceed */
static void
nautilus_wipe_fill_pop_dir (struct FillOperationData *opdata)
{
  GList *tmp;
  
  tmp = opdata->dir;
  opdata->dir = g_list_next (opdata->dir);
  g_free (tmp->data);
  g_list_free_1 (tmp);
}

/* Cleans up the opdata structure and frees it */
static void
nautilus_wipe_fill_cleanup (struct FillOperationData *opdata)
{
  g_signal_handler_disconnect (opdata->operation, opdata->progress_hid);
  g_signal_handler_disconnect (opdata->operation, opdata->finished_hid);
  if (opdata->operation) {
    g_object_unref (opdata->operation);
  }
  while (opdata->dir) {
    nautilus_wipe_fill_pop_dir (opdata);
  }
  g_slice_free1 (sizeof *opdata, opdata);
}

/* wrapper for the progress handler returning the current progression over all
 * operations  */
static void
nautilus_wipe_fill_progress_handler (GsdFillOperation         *operation,
                                     gdouble                   fraction,
                                     struct FillOperationData *opdata)
{
  opdata->progress_handler (operation,
                            (opdata->n_op_done + fraction) / opdata->n_op,
                            opdata->cbdata);
}

/* timeout function to launch next operation after finish of the previous
 * operation.
 * we need this kind of hack since operation are locked while running. */
static gboolean
launch_next_fill_operation (struct FillOperationData *opdata)
{
  gboolean busy;
  
  busy = gsd_async_operation_get_busy (GSD_ASYNC_OPERATION (opdata->operation));
  if (! busy) {
    GError   *err = NULL;
    gboolean  success;
    
    success = do_fill_operation (opdata, &err);
    if (! success) {
      nautilus_wipe_fill_finished_handler (opdata->operation,
                                           success, err->message, opdata);
      g_error_free (err);
    }
  }
  
  return busy; /* keeps our timeout function until lock is released */
}

/* Wrapper for the finished handler.
 * It launches the next operation if there is one left, or call the user's
 * handler if done or on error. */
static void
nautilus_wipe_fill_finished_handler (GsdFillOperation         *operation,
                                     gboolean                  success,
                                     const gchar              *message,
                                     struct FillOperationData *opdata)
{
  opdata->n_op_done++;
  /* remove the directory just proceeded */
  nautilus_wipe_fill_pop_dir (opdata);
  /* if the last operation succeeded and we have work left */
  if (success && opdata->dir) {
    /* we can't launch the next operation right here since the previous must
     * release its lock before, which is done just after return of the current
     * function.
     * To work around this, we add a timeout function that will try to launch
     * the next operation if the current one is not busy, which fixes the
     * problem. */
    g_timeout_add (10, (GSourceFunc)launch_next_fill_operation, opdata);
  } else {
    opdata->finished_handler (operation, success, message, opdata->cbdata);
    nautilus_wipe_fill_cleanup (opdata);
  }
}

/*
 * nautilus_wipe_fill_operation_filter_files:
 * @paths: A list of paths to filter
 * @work_paths_: return location for filtered paths
 * @work_mounts_: return location for filtered paths' mounts
 * @error: return location for errors, or %NULL to ignore them
 * 
 * Ties to get usable paths (local directories) and keep only one per
 * mountpoint.
 * 
 * The returned lists (@work_paths_ and @work_mounts_) have the same length, and
 * an index in a list correspond to the same in the other:
 * g_list_index(work_paths_, 0) is the path of g_list_index(work_mounts_, 0).
 * Free returned lists with nautilus_wipe_path_list_free().
 * 
 * Returns: %TRUE on success, %FALSE otherwise.
 */
gboolean
nautilus_wipe_fill_operation_filter_files (GList    *paths,
                                           GList   **work_paths_,
                                           GList   **work_mounts_,
                                           GError  **error)
{
  GList  *work_paths  = NULL;
  GError *err         = NULL;
  GList  *work_mounts = NULL;
  
  g_return_val_if_fail (paths != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);
  
  for (; ! err && paths; paths = g_list_next (paths)) {
    const gchar  *file_path = paths->data;
    gchar        *mountpoint;
    
    mountpoint = find_mountpoint (file_path, &err);
    if (G_LIKELY (mountpoint)) {
      if (g_list_find_custom (work_mounts, mountpoint, (GCompareFunc)strcmp)) {
        /* the mountpoint is already added, skip it */
        g_free (mountpoint);
      } else {
        gchar *path;
        
        work_mounts = g_list_prepend (work_mounts, mountpoint);
        /* if it is not a directory, gets its container directory.
         * no harm since files cannot be mountpoint themselves, then it gets
         * at most the mountpoint itself */
        if (! g_file_test (file_path, G_FILE_TEST_IS_DIR)) {
          path = g_path_get_dirname (file_path);
        } else {
          path = g_strdup (file_path);
        }
        work_paths = g_list_prepend (work_paths, path);
      }
    }
  }
  if (err || ! work_paths_) {
    nautilus_wipe_path_list_free (work_paths);
  } else {
    *work_paths_ = g_list_reverse (work_paths);
  }
  if (err || ! work_mounts_) {
    nautilus_wipe_path_list_free (work_mounts);
  } else {
    *work_mounts_ = g_list_reverse (work_mounts);
  }
  if (err) {
    g_propagate_error (error, err);
  }
  
  return ! err;
}

/*
 * nautilus_wipe_fill_operation:
 * @directories: A list of paths to work on (should have been filtered with
 *               nautilus_wipe_fill_operation_filter_files() or so)
 * @fast: The Gsd.SecureDeleteOperation:fast setting
 * @mode: The Gsd.SecureDeleteOperation:mode setting
 * @zeroise: The Gsd.ZeroableOperation:zeroise setting
 * @finished_handler: A handler for GsdAsyncOperation::finished
 * @progress_handler: A handler for GsdAsyncOperation::progress
 * @data: User data to pass to @finished_handler and @progress_handler
 * @error: Return location for errors or %NULL to ignore them. The errors are
 *         those from gsd_fill_operation_run().
 * 
 * fills the given directories to overwrite available diskspace.
 * 
 * Returns: The operation object that was launched, or %NULL on failure.
 *          The operation object should be unref'd with g_object_unref() when
 *          no longer needed.
 */
GsdAsyncOperation *
nautilus_wipe_fill_operation (GList                       *directories,
                              gboolean                     fast,
                              GsdSecureDeleteOperationMode mode,
                              gboolean                     zeroise,
                              GCallback                    finished_handler,
                              GCallback                    progress_handler,
                              gpointer                     data,
                              GError                     **error)
{
  gboolean                  success = FALSE;
  struct FillOperationData *opdata;
  GList                    *dirs;
  
  g_return_val_if_fail (directories != NULL, NULL);
  
  dirs = nautilus_wipe_path_list_copy (directories);
  if (dirs) {
    opdata = g_slice_alloc (sizeof *opdata);
    opdata->dir               = dirs;
    opdata->finished_handler  = (FillFinishedFunc)finished_handler;
    opdata->progress_handler  = (FillProgressFunc)progress_handler;
    opdata->cbdata            = data;
    opdata->n_op              = g_list_length (opdata->dir);
    opdata->n_op_done         = 0;
    opdata->operation         = gsd_fill_operation_new ();
    gsd_secure_delete_operation_set_fast (GSD_SECURE_DELETE_OPERATION (opdata->operation), fast);
    gsd_secure_delete_operation_set_mode (GSD_SECURE_DELETE_OPERATION (opdata->operation), mode);
    gsd_zeroable_operation_set_zeroise (GSD_ZEROABLE_OPERATION (opdata->operation), zeroise);
    opdata->progress_hid = g_signal_connect (opdata->operation, "progress",
                                             G_CALLBACK (nautilus_wipe_fill_progress_handler), opdata);
    opdata->finished_hid = g_signal_connect (opdata->operation, "finished",
                                             G_CALLBACK (nautilus_wipe_fill_finished_handler), opdata);
    /* launches the operation */
    success = do_fill_operation (opdata, error);
    if (! success) {
      nautilus_wipe_fill_cleanup (opdata);
    }
  }
  
  return success ? g_object_ref (opdata->operation) : NULL;
}

