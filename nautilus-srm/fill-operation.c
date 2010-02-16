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

#include <gio/gio.h>
#if HAVE_GIO_UNIX
# include <gio/gunixmounts.h>
#endif

#include <gsecuredelete/gsecuredelete.h>
#include <libnautilus-extension/nautilus-file-info.h>


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
    
    //~ g_debug ("trying %s", mountpoint);
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
find_mountpoint (GFile *file)
{
  gchar  *mountpoint_path = NULL;
  GMount *mount;
  
  /* Try with GIO first */
  mount = g_file_find_enclosing_mount (file, NULL, NULL);
  if (mount) {
    GFile *mountpoint_file;
    
    mountpoint_file = g_mount_get_root (mount);
    mountpoint_path = g_file_get_path (mountpoint_file);
    g_object_unref (mountpoint_file);
    g_object_unref (mount);
  }
  #if HAVE_GIO_UNIX
  /* fallback to find_unix_mount() */
  if (! mountpoint_path) {
    gchar *path = g_file_get_path (file);
    
    if (path) {
      mountpoint_path = find_mountpoint_unix (path);
    }
    g_free (path);
  }
  #endif
  
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

/* Actually calls libgsecuredelete */
static gboolean
do_sfill_operation (struct FillOperationData *opdata,
                    GError                  **error)
{
  /* FIXME: don't launch sfill in a useful directory since it can leave a bunch
   * of files if it get interrupted (e.g. from a crash, a user kill or so) */
  return gsd_fill_operation_run (opdata->operation, opdata->dir->data, 100, error);
}

/* Removes the current directory to proceed */
static void
nautilus_srm_fill_pop_dir (struct FillOperationData *opdata)
{
  GList *tmp;
  
  tmp = opdata->dir;
  opdata->dir = g_list_next (opdata->dir);
  g_free (tmp->data);
  g_list_free_1 (tmp);
}

/* Cleans up the opdata structure and frees it */
static void
nautilus_srm_fill_cleanup (struct FillOperationData *opdata)
{
  g_signal_handler_disconnect (opdata->operation, opdata->progress_hid);
  g_signal_handler_disconnect (opdata->operation, opdata->finished_hid);
  g_object_unref (opdata->operation);
  while (opdata->dir) {
    nautilus_srm_fill_pop_dir (opdata);
  }
  g_slice_free1 (sizeof *opdata, opdata);
}

/* wrapper for the progress handler returning the current progression over all
 * operations  */
static void
nautilus_srm_fill_progress_handler (GsdFillOperation         *operation,
                                    gdouble                   fraction,
                                    struct FillOperationData *opdata)
{
  opdata->progress_handler (operation,
                            (opdata->n_op_done + fraction) / opdata->n_op,
                            opdata->cbdata);
}

/* Wrapper for the finished handler.
 * It launches the next operation if there is one left, or call the user's
 * handler if done or on error. */
static void
nautilus_srm_fill_finished_handler (GsdFillOperation         *operation,
                                    gboolean                  success,
                                    const gchar              *message,
                                    struct FillOperationData *opdata)
{
  gboolean free_message = FALSE; /* hack to be able to fill @message */
  
  opdata->n_op_done++;
  /* remove the directory just proceeded */
  nautilus_srm_fill_pop_dir (opdata);
  /* if the last operation succeeded and we have work left */
  if (success && opdata->dir) {
    GError *err = NULL;
    
    success = do_sfill_operation (opdata, &err);
    if (! success) {
      message = g_strdup (err->message);
      free_message = TRUE;
      g_error_free (err);
    }
  }
  if (! success || ! opdata->dir) {
    opdata->finished_handler (operation, success, message, opdata->cbdata);
    nautilus_srm_fill_cleanup (opdata);
  }
  if (free_message) {
    g_free ((gchar *)message);
  }
}

/*
 * filter_dir_list:
 * @directories: a list of #NautilusFileInfo
 * 
 * Filter the input file list so that there remains only one path for
 * each filesystem.
 * 
 * Returns: A #GList of file pathes to actually work on. Free each element with
 *          g_free() before freeing the list.
 */
static GList *
filter_dir_list (GList *directories)
{
  GList      *dirs = NULL;
  /* table of different mountpoints */
  GHashTable *mountpoints = g_hash_table_new_full (g_str_hash, g_str_equal,
                                                   g_free, NULL);
  
  for (; directories; directories = g_list_next (directories)) {
    GFile *file = nautilus_file_info_get_location (directories->data);
    gchar *mountpoint;
    
    mountpoint = find_mountpoint (file);
    if (G_UNLIKELY (! mountpoint)) {
      /* FIXME: do something even if this is very unlikely */
      gchar *uri = g_file_get_uri (file);
      
      g_warning ("Cannot find mountpoint of URI %s, skipping", uri);
      g_free (uri);
    } else {
      if (g_hash_table_lookup (mountpoints, mountpoint)) {
        /* the mountpoint is already added, skip it */
        g_free (mountpoint);
      } else {
        gchar *path;
        
        g_hash_table_insert (mountpoints, mountpoint, NULL);
        path = g_file_get_path (file);
        if (! path) {
          /* FIXME: do something (it is probably a remote file)
           * hum, I think it can't happen since find_mountpoint() would fail
           * on remote files */
          gchar *uri = g_file_get_uri (file);
          
          g_warning ("Cannot get path for URI %s, skipping", uri);
          g_free (uri);
        } else {
          dirs = g_list_append (dirs, path);
        }
      }
    }
    g_object_unref (file);
  }
  g_hash_table_destroy (mountpoints);
  
  return dirs;
}

/*
 * nautilus_srm_fill_operation:
 * @directories: A list of #NautilusFileInfo to work on
 * @finished_handler: A handler for GsdAsyncOperation::finished
 * @progress_handler: A handler for GsdAsyncOperation::progress
 * @data: User data to pass to @finished_handler and @progress_handler
 * @error: Return location for errors or %NULL to ignore them. The errors are
 *         those from gsd_fill_operation_run().
 * 
 * "sfill"s the given directories.
 * 
 * Returns: %TRUE if operation successfully started, %FALSE otherwise. %TRUE
 *          does not mean that anything was actually done, but only that the
 *          operation started successfully.
 */
gboolean
nautilus_srm_fill_operation (GList    *directories,
                             GCallback finished_handler,
                             GCallback progress_handler,
                             gpointer  data,
                             GError  **error)
{
  gboolean                  success = TRUE;
  struct FillOperationData *opdata;
  
  opdata = g_slice_alloc (sizeof *opdata);
  opdata->dir               = filter_dir_list (directories);
  opdata->finished_handler  = (FillFinishedFunc)finished_handler;
  opdata->progress_handler  = (FillProgressFunc)progress_handler;
  opdata->cbdata            = data;
  opdata->n_op              = g_list_length (opdata->dir);
  opdata->n_op_done         = 0;
  opdata->operation         = gsd_fill_operation_new ();
  opdata->progress_hid = g_signal_connect (opdata->operation, "progress",
                                           G_CALLBACK (nautilus_srm_fill_progress_handler), opdata);
  opdata->finished_hid = g_signal_connect (opdata->operation, "finished",
                                           G_CALLBACK (nautilus_srm_fill_finished_handler), opdata);
  /* launches the operation */
  success = do_sfill_operation (opdata, error);
  if (! success) {
    nautilus_srm_fill_cleanup (opdata);
  }
  
  return success;
}

