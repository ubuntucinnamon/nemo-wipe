/*
 *  nautilus-srm - a nautilus extension to wipe file(s) with srm
 * 
 *  Copyright (C) 2009 Colomban Wendling <ban@herbesfolles.org>
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

#include "nautilus-srm.h"

#include <libnautilus-extension/nautilus-menu-provider.h>
#include <libnautilus-extension/nautilus-file-info.h>

#include <glib.h>
#define GETTEXT_PACKAGE "nautilus-srm"
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>

/* if GLib doesn't provide g_dngettext(), wrap it from dngettext() */
#if (! GLIB_CHECK_VERSION (2, 18, 0) && ! defined (g_dngettext))
# include <libintl.h>
# define g_dngettext dngettext
#endif


struct _NautilusSrmPrivate {
  GtkWindow *parent_window;
  GList     *files;
};
typedef struct _NautilusSrmPrivate NautilusSrmPrivate;

#define GET_PRIVATE(obj)  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), NAUTILUS_TYPE_SRM, NautilusSrmPrivate))

static GType provider_types[1];
static GType nautilus_srm_type = 0;

/* private prototypes */
void          nautilus_module_initialize     (GTypeModule *module);
void          nautilus_srm_register_type     (GTypeModule *module);
static GList *nautilus_srm_get_file_items    (NautilusMenuProvider *provider,
                                              GtkWidget            *window,
                                              GList                *files);

/*=== Nautilus interface functions ===*/

/* Initialize our extension */
void
nautilus_module_initialize (GTypeModule *module)
{
  g_message (_("Initializing"));
  nautilus_srm_register_type (module);
  provider_types[0] = nautilus_srm_get_type ();
}

/* The next function returns the type of object Nautilus needs to create for your extension. */

void
nautilus_module_list_types (const GType **types,
                            int          *num_types)
{
  *types = provider_types;
  *num_types = G_N_ELEMENTS (provider_types);
}

/*Then comes the function that handles any tasks required to shut the Extension down cleanly.*/

void
nautilus_module_shutdown (void)
{
  g_message (_("Shutting down"));
  /* Any module-specific shutdown code*/
}


/*=== Type registration ===*/

GQuark
nautilus_srm_error_quark (void)
{
  static GQuark error_quark = 0;
  
  if (G_UNLIKELY (error_quark == 0)) {
    error_quark = g_quark_from_static_string ("NautilusSrmError");
  }
  
  return error_quark;
}

static void
nautilus_srm_menu_provider_iface_init (NautilusMenuProviderIface *iface)
{
  iface->get_file_items = nautilus_srm_get_file_items;
}

/* re-sets ths private data. If new value is NULL, the value is only freed */
static inline void
nautilus_srm_reset_private (NautilusSrm *srm,
                            GtkWindow   *window,
                            GList       *files)
{
  NautilusSrmPrivate *priv = GET_PRIVATE (srm);
  
  /* unset */
  if (priv->parent_window) {
    /*g_object_unref (priv->parent_window);*/
    priv->parent_window = NULL;
  }
  if (priv->files) {
    nautilus_file_info_list_free (priv->files);
    priv->files = NULL;
  }
  /* then set */
  if (window) {
    priv->parent_window = /*g_object_ref*/ (window);
  }
  if (files) {
    priv->files = nautilus_file_info_list_copy (files);
  }
}

static void 
nautilus_srm_instance_init (NautilusSrm *srm)
{
  NautilusSrmPrivate *priv = GET_PRIVATE (srm);
  
  priv->parent_window = NULL;
  priv->files = NULL;
  g_message ("Object [%p] initialized", srm);
}

static void 
nautilus_srm_instance_finalize (GObject *object)
{
  nautilus_srm_reset_private (NAUTILUS_SRM (object), NULL, NULL);
  g_message ("Object [%p] finalized", object);
}

static void
nautilus_srm_class_init (NautilusSrmClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  
  object_class->finalize = nautilus_srm_instance_finalize;
  
  g_type_class_add_private (class, sizeof (NautilusSrmPrivate));
  
  g_message ("Class initialized");
}

static void
nautilus_srm_class_finalize (NautilusSrmClass *class)
{
  g_message ("Class finalized");
}

GType
nautilus_srm_get_type (void)
{
  return nautilus_srm_type;
}



/* Register our type into glib */
void
nautilus_srm_register_type (GTypeModule *module)
{
  static const GTypeInfo info = {
    sizeof (NautilusSrmClass),
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) nautilus_srm_class_init,
    NULL,
    NULL,
    sizeof (NautilusSrm),
    0,
    (GInstanceInitFunc) nautilus_srm_instance_init,
  };
  /* Nautilus Menu Provider Interface */
  static const GInterfaceInfo menu_provider_iface_info = {
    (GInterfaceInitFunc) nautilus_srm_menu_provider_iface_init,
     NULL,
     NULL
  };
  
  nautilus_srm_type = g_type_module_register_type (module,
                                                    G_TYPE_OBJECT,
                                                    "NautilusSrm",
                                                    &info, 0);
  
  g_type_module_add_interface (module, nautilus_srm_type,
                               NAUTILUS_TYPE_MENU_PROVIDER,
                               &menu_provider_iface_info);
  
}



/*=== Actual extension ===*/

static void     menu_activate_cb             (NautilusMenuItem  *menu,
                                              NautilusSrm *srm);
static gboolean do_srm                       (GList      *files,
                                              GtkWindow  *parent_window,
                                              GError    **error);


static GList *
nautilus_srm_get_file_items (NautilusMenuProvider *provider,
                             GtkWidget            *window,
                             GList                *files)
{
  NautilusMenuItem *item;
  GList            *items = NULL;
  
  item = nautilus_menu_item_new ("NautilusSrm::srm_file_item",
                                 _("Definitely delete"),
                                 g_dngettext (NULL, "Definitely delete the selected file",
                                                    "Definitely delete the selected files",
                                                    g_list_length (files)),
                                 GTK_STOCK_DELETE);
  items = g_list_append (items, item);
  
  /* fill the object's private fields */
  nautilus_srm_reset_private (NAUTILUS_SRM (provider),
                              GTK_WINDOW (window), files);
  
  g_signal_connect (item, "activate", G_CALLBACK (menu_activate_cb), provider);
  
  return items;
}

static void
menu_activate_cb (NautilusMenuItem *menu,
                  NautilusSrm      *srm)
{
  NautilusSrmPrivate *priv = GET_PRIVATE (srm);
  GtkWidget *dialog;
  GList *list_item;
  GString *files_names = g_string_new ("");
  gchar *files_names_str;
  int user_response;
  gint n_files = 0;
    
  for (list_item = priv->files; list_item != NULL; list_item = g_list_next (list_item))
  {
    gchar *name;
    
    name = nautilus_file_info_get_name (list_item->data);
    g_string_append (files_names, name);
    if (list_item->next != NULL)
      g_string_append (files_names, ", ");
    g_free (name);
    n_files ++;
  }
  files_names_str = g_string_free (files_names, FALSE);
  
  dialog = gtk_message_dialog_new (priv->parent_window, 
                                   GTK_DIALOG_MODAL,
                                   GTK_MESSAGE_WARNING,
                                   GTK_BUTTONS_NONE,
    g_dngettext(NULL, "Are you sure you want to securely and definitely delete the following file?",
                      "Are you sure you want to securely and definitely delete the following files?",
                      n_files));
  gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
                                            "%s",
                                            files_names_str);
  gtk_dialog_add_buttons (GTK_DIALOG (dialog),
                          GTK_STOCK_CANCEL, GTK_RESPONSE_NO,
                          GTK_STOCK_DELETE, GTK_RESPONSE_YES,
                          NULL);
  user_response = gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
  
  if (user_response == GTK_RESPONSE_YES)
  {
    GError *err = NULL;
    
    if (! do_srm (priv->files, priv->parent_window, &err)) {
      dialog = gtk_message_dialog_new (priv->parent_window, 
                                       GTK_DIALOG_MODAL,
                                       GTK_MESSAGE_ERROR,
                                       GTK_BUTTONS_CLOSE,
                                       g_dngettext(NULL, "Failed to delete file",
                                                         "Failed to delete some files",
                                                         n_files));
      gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
                                                "%s", err->message);
      gtk_dialog_run (GTK_DIALOG (dialog));
      gtk_widget_destroy (dialog);
      
      g_error_free (err);
    }
  }
  
  g_free (files_names_str);
}


typedef struct _ProgressDialog {
  GtkWindow      *window;
  GtkProgressBar *progress_bar;
} ProgressDialog;

static ProgressDialog *
build_progress_dialog (const gchar *title,
                       GtkWindow   *parent,
                       const gchar *format,
                       ...)
{
  ProgressDialog *dialog;
  va_list         ap;
  gchar          *text;
  GtkWidget      *label;
  GtkWidget      *box;
  
  dialog = g_new0 (ProgressDialog, 1);
  dialog->window = GTK_WINDOW (gtk_window_new (GTK_WINDOW_TOPLEVEL));
  gtk_window_set_title (dialog->window, title);
  gtk_window_set_deletable (dialog->window, FALSE);
  gtk_window_set_transient_for (dialog->window, parent);
  gtk_window_set_position (dialog->window, GTK_WIN_POS_CENTER_ON_PARENT);
  gtk_window_set_skip_pager_hint (dialog->window, TRUE);
  gtk_window_set_skip_taskbar_hint (dialog->window, TRUE);
  gtk_container_set_border_width (GTK_CONTAINER (dialog->window), 10);
  
  dialog->progress_bar = GTK_PROGRESS_BAR (gtk_progress_bar_new ());
  gtk_progress_bar_set_pulse_step (dialog->progress_bar, 0.1);
  
  va_start (ap, format);
  text = g_strdup_vprintf (format, ap);
  va_end (ap);
  label = gtk_label_new (text);
  g_free (text);
  
  box = gtk_vbox_new (FALSE, 10);
  gtk_box_pack_start (GTK_BOX (box), label, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (box), GTK_WIDGET (dialog->progress_bar), FALSE, FALSE, 0);
  gtk_container_add (GTK_CONTAINER (dialog->window), box);
  
  gtk_widget_show_all (box);
  
  return dialog;
}

static void
destroy_progress_dialog (ProgressDialog *dialog)
{
  gtk_widget_destroy (GTK_WIDGET (dialog->window));
  g_free (dialog);
}


/* readstrdup:
 * 
 * @fd: a file descriptor
 * @n_bytes: number of bytes to read from @fd, or -1 to read all @fd
 * 
 * Reads content of a file as a C string (0-terminated).
 * 
 * Returns: a newly allocated string containing data in @fd that should be freed
 *          when no longer needed using g_free().
 */
static gchar *
readstrdup (int     fd,
            gssize  n_bytes)
{
  gchar *buf = NULL;
  
  if (n_bytes > 0) {
    gssize n_read;
    
    buf = g_malloc (n_bytes + 1);
    n_read = read (fd, buf, n_bytes);
    if (n_read < 0) {
      g_free (buf), buf = NULL;
    } else {
      buf[n_read] = 0;
    }
  } else {
    static const gsize packet_size = 64;
    gsize              n_read = 0;
    gssize read_rv;
    
    n_bytes = 0;
    do {
      buf = g_realloc (buf, n_bytes + packet_size + 1);
      
      read_rv = read (fd, &buf[n_bytes], packet_size);
      if (read_rv >= 0) {
        n_read += read_rv;
      }
      
      n_bytes += packet_size;
    } while (n_bytes > 0 /* don't underflow */ &&
             read_rv >= 0 &&
             read_rv == packet_size);
    
    if (read_rv < 0) {
      g_free (buf), buf = NULL;
    } else {
      buf[n_read] = 0;
    }
  }
  
  return buf;
}

typedef struct _SrmChildInfo {
  gchar **argv; /* just to be able to free it */
  gint    fd_out;
  gint    fd_err;
  GPid    pid;
  
  ProgressDialog *progress_dialog;
  GtkWindow *parent_window;
} SrmChildInfo;

/* waits for the child process to finish and display a dialog on error
 * It also display a progress dialog for the user to know something is
 * happening.
 * 
 * This function is designed to be used as a GSourceFunc function. */
static gboolean
wait_srm_child (gpointer data)
{
  SrmChildInfo *child_info = data;
  gboolean finished = TRUE;
  gboolean success  = FALSE;
  int      exit_status;
  pid_t    wait_rv;
  
  wait_rv = waitpid (child_info->pid, &exit_status, WNOHANG);
  if (G_UNLIKELY (wait_rv < 0)) {
    g_warning ("waitpid() failed: %s", g_strerror (errno));
    /* if we cannot watch the child, try to kill it */
    if (kill (child_info->pid, 15) < 0) {
      g_warning ("kill() failed: %s", g_strerror (errno));
    } else {
      g_message ("Subprocess killed.");
    }
  } else if (G_LIKELY (wait_rv == 0)) {
    /* nothing to do, just wait until next call */
    gtk_progress_bar_pulse (child_info->progress_dialog->progress_bar);
    finished = FALSE;
  } else {
    if (WIFEXITED (exit_status) && WEXITSTATUS (exit_status) == 0) {
      success = TRUE;
      g_message ("Subprocess succeed.");
    } else {
      g_message ("Subprocess failed.");
    }
  }
  
  if (G_UNLIKELY (finished)) {
    destroy_progress_dialog (child_info->progress_dialog);
    
    if (! success) {
      GtkWidget *dialog;
      gchar     *error_output = NULL;
      
      error_output = readstrdup (child_info->fd_err, -1);
      
      dialog = gtk_message_dialog_new (child_info->parent_window, 
                                       GTK_DIALOG_MODAL,
                                       GTK_MESSAGE_ERROR,
                                       GTK_BUTTONS_CLOSE,
                                       _("Suppression failed"));
      gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
                                                "%s", error_output);
      gtk_dialog_run (GTK_DIALOG (dialog));
      gtk_widget_destroy (dialog);
      
      g_free (error_output);
    }
    
    /* cleanup */
    g_spawn_close_pid (child_info->pid);
    close (child_info->fd_err);
    close (child_info->fd_out);
    g_strfreev (child_info->argv);
    g_object_unref (child_info->parent_window);
    g_free (child_info);
  }
  
  return ! finished;
}

static gboolean
do_srm (GList      *files,
        GtkWindow  *parent_window,
        GError    **error)
{
  GList    *file;
  gchar   **argv;
  int       i = 0;
  gboolean  success = TRUE;
  
  argv = g_new0 (gchar *, g_list_length (files) + 1 + 1 /* number of args */ + 1);
  argv[i++] = g_strdup ("srm");
  argv[i++] = g_strdup ("-r");
  
  for (file = files; success && file != NULL; file = g_list_next (file))
  {
    gchar *scheme;
    
    scheme = nautilus_file_info_get_uri_scheme (file->data);
    if (strcmp (scheme, "file") == 0) {
      gchar *escaped_uri;
      gchar *uri;
      
      uri = nautilus_file_info_get_uri (file->data);
      escaped_uri = g_uri_unescape_string (uri, NULL);
      /* strlen (file://) = 7 */
      argv[i++] = g_strdup (&escaped_uri[7]);
      
      g_free (escaped_uri);
      g_free (uri);
    } else {
      g_set_error (error, NAUTILUS_SRM_ERROR, NAUTILUS_SRM_ERROR_UNSUPPORTED_LOCATION,
                   _("Unsupported location type: %s"), scheme);
      success = FALSE;
    }
    g_free (scheme);
  }
  argv[i] = NULL;
  
  if (success) {
    GError *err = NULL;
    SrmChildInfo *child_info;
    
    child_info = g_new0 (SrmChildInfo, 1);
    child_info->argv = argv; /* we let the child free its args as the GLib
                              * doesn't seems to manage or free it */
    child_info->fd_out = -1;
    child_info->fd_err = -1;
    child_info->parent_window = g_object_ref (parent_window);
    
    if (! g_spawn_async_with_pipes (NULL, argv, NULL,
                                    G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD,
                                    NULL, NULL,
                                    &child_info->pid,
                                    NULL,
                                    &child_info->fd_out, &child_info->fd_err,
                                    &err)) {
      g_set_error (error, NAUTILUS_SRM_ERROR, NAUTILUS_SRM_ERROR_SPAWN_FAILED,
                   _("Failed to spawn subprocess: %s"),
                   err->message);
      g_error_free (err);
      success = FALSE;
    } else {
      g_message ("Suppressing...");
      
      child_info->progress_dialog = build_progress_dialog (_("Progress"), parent_window, _("Removing files..."));
      gtk_widget_show (GTK_WIDGET (child_info->progress_dialog->window));
      /* add timeout function */
      g_timeout_add (100, wait_srm_child, child_info);
    }
    
    if (! success) {
      g_free (child_info);
    }
  }
  
  if (! success) {
    g_strfreev (argv);
  }
  
  g_message ("end of caller");
  
  return success;
}












