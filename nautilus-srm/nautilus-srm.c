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

#include <gsecuredelete/gsecuredelete.h>

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

static void
progress_dialog_set_fraction (ProgressDialog *dialog,
                              gdouble         fraction)
{
  gchar *text;
  
  gtk_progress_bar_set_fraction (dialog->progress_bar, fraction);
  text = g_strdup_printf ("%.0f%%", fraction * 100);
  gtk_progress_bar_set_text (dialog->progress_bar, text);
  g_free (text);
}

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
  progress_dialog_set_fraction (dialog, 0.0);
  
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


typedef struct _SrmCbData {
  ProgressDialog *progress_dialog;
  GtkWindow      *parent_window;
} SrmCbData;

/* callback for progress notification */
static void
operation_progress (SecureDeleteDeleteOperation  *operation,
                    gdouble                       progress,
                    SrmCbData                    *cbdata)
{
  /*g_message ("progress is now %.0f%%.", progress * 100);*/
  progress_dialog_set_fraction (cbdata->progress_dialog, progress);
}

/* callback for the finished signal */
static void
operation_finished (SecureDeleteDeleteOperation  *operation,
                    gboolean                      success,
                    const gchar                  *error_message,
                    SrmCbData                    *cbdata)
{
  destroy_progress_dialog (cbdata->progress_dialog);
  if (! success) {
    GtkWidget *dialog;
    gchar     *error_output = NULL;
    
    dialog = gtk_message_dialog_new (cbdata->parent_window, 
                                     GTK_DIALOG_MODAL,
                                     GTK_MESSAGE_ERROR,
                                     GTK_BUTTONS_CLOSE,
                                     _("Suppression failed"));
    gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
                                              "%s", error_message);
    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
  }
  /* cleanup */
  g_object_unref (cbdata->parent_window);
  g_free (cbdata);
}

/* Adds the file_infos to the operation. Fails if not supported. */
static gboolean
add_nautilus_file_infos (SecureDeleteDeleteOperation *operation,
                         GList                       *file_infos,
                         GError                     **error)
{
  gboolean  success = TRUE;
  GList    *info;
  
  for (info = file_infos; success && info != NULL; info = g_list_next (info))
  {
    gchar *scheme;
    
    scheme = nautilus_file_info_get_uri_scheme (info->data);
    if (strcmp (scheme, "file") == 0) {
      gchar *escaped_uri;
      gchar *uri;
      
      uri = nautilus_file_info_get_uri (info->data);
      escaped_uri = g_uri_unescape_string (uri, NULL);
      /* strlen (file://) = 7 */
      secure_delete_delete_operation_add_path (operation, &escaped_uri[7]);
      
      g_free (escaped_uri);
      g_free (uri);
    } else {
      g_set_error (error, NAUTILUS_SRM_ERROR, NAUTILUS_SRM_ERROR_UNSUPPORTED_LOCATION,
                   _("Unsupported location type: %s"), scheme);
      success = FALSE;
    }
    g_free (scheme);
  }
  
  return success;
}

static gboolean
do_srm (GList      *files,
        GtkWindow  *parent_window,
        GError    **error)
{
  GList    *file;
  int       i = 0;
  gboolean  success = TRUE;
  SecureDeleteDeleteOperation *operation;
  
  operation = secure_delete_delete_operation_new ();
  success = add_nautilus_file_infos (operation, files, error);
  if (success) {
    GError *err = NULL;
    SrmCbData *cbdata;
    
    cbdata = g_new0 (SrmCbData, 1);
    cbdata->parent_window = g_object_ref (parent_window);
    cbdata->progress_dialog = build_progress_dialog (_("Progress"),
                                                     parent_window,
                                                     _("Removing files..."));
    gtk_widget_show (GTK_WIDGET (cbdata->progress_dialog->window));
    
    g_signal_connect (operation, "finished", operation_finished, cbdata);
    g_signal_connect (operation, "progress", operation_progress, cbdata);
    
    if (! secure_delete_secure_delete_operation_run (
            SECURE_DELETE_SECURE_DELETE_OPERATION (operation),
            100,
            &err)) {
      g_set_error (error, NAUTILUS_SRM_ERROR, NAUTILUS_SRM_ERROR_SPAWN_FAILED,
                   _("Failed to spawn subprocess: %s"),
                   err->message);
      g_error_free (err);
      success = FALSE;
    } /*else {
      g_message ("Suppressing...");
    }*/
    
    if (! success) {
      destroy_progress_dialog (cbdata->progress_dialog);
      g_object_unref (cbdata->parent_window);
      g_free (cbdata);
    }
  }
  
  return success;
}












