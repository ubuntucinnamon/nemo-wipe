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

#include "operation-manager.h"

#include <stdarg.h>
#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <gsecuredelete/gsecuredelete.h>

#include "nautilus-srm.h"
#include "progress-dialog.h"
#include "compat.h"


static GtkResponseType  display_dialog     (GtkWindow       *parent,
                                            GtkMessageType   type,
                                            gboolean         wait_for_response,
                                            const gchar     *primary_text,
                                            const gchar     *secondary_text,
                                            const gchar     *first_button_text,
                                            ...) G_GNUC_NULL_TERMINATED;

/*
 * display_dialog:
 * @parent: Parent window, or %NULL
 * @type: The dialog type
 * @wait_for_response: Whether to wait for the dialog's response. Waiting for
 *                     the response force a modal-like dialog (using
 *                     gtk_dialog_run()), but allow to get the dialog's
 *                     response. If this options is %TRUE, this function will
 *                     always return %GTK_RESPONSE_NONE.
 *                     Use this if you want a modal dialog.
 * @primary_text: GtkMessageDialog's primary text
 * @secondary_text: GtkMessageDialog's secondary text, or %NULL
 * @first_button_text: Text of the first button, or %NULL
 * @...: (starting at @first_button_text) %NULL-terminated list of buttons text
 *       and response-id.
 * 
 * Returns: The dialog's response or %GTK_RESPONSE_NONE if @wait_for_response
 *          is %FALSE.
 */
static GtkResponseType
display_dialog (GtkWindow       *parent,
                GtkMessageType   type,
                gboolean         wait_for_response,
                const gchar     *primary_text,
                const gchar     *secondary_text,
                const gchar     *first_button_text,
                ...)
{
  GtkResponseType response = GTK_RESPONSE_NONE;
  GtkWidget      *dialog;
  va_list         ap;
  
  dialog = gtk_message_dialog_new (parent,
                                   GTK_DIALOG_DESTROY_WITH_PARENT,
                                   type, GTK_BUTTONS_NONE,
                                   "%s", primary_text);
  if (secondary_text) {
    gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
                                              "%s", secondary_text);
  }
  va_start (ap, first_button_text);
  while (first_button_text) {
    GtkResponseType button_response = va_arg (ap, GtkResponseType);
    
    gtk_dialog_add_button (GTK_DIALOG (dialog), first_button_text, button_response);
    first_button_text = va_arg (ap, const gchar *);
  }
  va_end (ap);
  /* show the dialog */
  if (wait_for_response) {
    response = gtk_dialog_run (GTK_DIALOG (dialog));
    /* if not already destroyed by the parent */
    if (GTK_IS_WIDGET (dialog)) {
      gtk_widget_destroy (dialog);
    }
  } else {
    g_signal_connect (dialog, "response", G_CALLBACK (gtk_widget_destroy), NULL);
    gtk_widget_show (dialog);
  }
  
  return response;
}



struct NautilusSrmOperationData
{
  GsdAsyncOperation          *operation;
  GtkWindow                  *window;
  gulong                      window_destroy_hid;
  NautilusSrmProgressDialog  *progress_dialog;
  gchar                      *failed_primary_text;
  gchar                      *success_primary_text;
  gchar                      *success_secondary_text;
  /*GList                      *files;*/
};

/* Frees a NautilusSrmOperationData structure */
static void
free_opdata (struct NautilusSrmOperationData *opdata)
{
  if (opdata->window_destroy_hid) {
    g_signal_handler_disconnect (opdata->window, opdata->window_destroy_hid);
  }
  g_object_unref (opdata->operation);
  g_free (opdata->failed_primary_text);
  g_free (opdata->success_primary_text);
  g_free (opdata->success_secondary_text);
  g_slice_free1 (sizeof *opdata, opdata);
}

/* if the parent window get destroyed, we honor gently the thing and leave it
 * to the death. doing this is useful not to have a bad window pointer later */
static void
opdata_window_destroy_handler (GtkObject                       *obj,
                               struct NautilusSrmOperationData *opdata)
{
  g_signal_handler_disconnect (opdata->window, opdata->window_destroy_hid);
  opdata->window_destroy_hid = 0;
  opdata->window = NULL;
}

/* Displays an operation's error */
static void
display_operation_error (struct NautilusSrmOperationData *opdata,
                         const gchar                     *error)
{
  display_dialog (opdata->window, GTK_MESSAGE_ERROR, FALSE,
                  opdata->failed_primary_text, error,
                  GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
                  NULL);
}

static void
operation_finished_handler (GsdDeleteOperation *operation,
                            gboolean            success,
                            const gchar        *error,
                            gpointer            data)
{
  struct NautilusSrmOperationData *opdata = data;
  
  gtk_widget_destroy (GTK_WIDGET (opdata->progress_dialog));
  if (! success) {
    display_operation_error (opdata, error);
  } else {
    display_dialog (opdata->window, GTK_MESSAGE_INFO, FALSE,
                    opdata->success_primary_text,
                    opdata->success_secondary_text,
                    GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
                    NULL);
  }
  free_opdata (opdata);
}

static void
operation_progress_handler (GsdDeleteOperation *operation,
                            gdouble             fraction,
                            gpointer            data)
{
  struct NautilusSrmOperationData *opdata = data;
  
  nautilus_srm_progress_dialog_set_fraction (opdata->progress_dialog,
                                             fraction);
}

/* sets @pref according to state of @toggle */
static void
pref_bool_toggle_changed_handler (GtkToggleButton *toggle,
                                  gboolean        *pref)
{
  *pref = gtk_toggle_button_get_active (toggle);
}

/* sets @pref to the value of the selected row, column 0 of combo's model */
static void
pref_enum_combo_changed_handler (GtkComboBox *combo,
                                 gint        *pref)
{
  GtkTreeIter   iter;
  
  if (gtk_combo_box_get_active_iter (combo, &iter)) {
    GtkTreeModel *model = gtk_combo_box_get_model (combo);
    
    gtk_tree_model_get (model, &iter, 0, pref, -1);
  }
}

/*
 * operation_confirm_dialog:
 * @parent: Parent window, or %NULL for none
 * @primary_text: Dialog's primary text
 * @secondary_text: Dialog's secondary text
 * @confirm_button_text: Text of the button to hit in order to confirm (can be a
 *                       stock item)
 * @fast: return location for the Gsd.SecureDeleteOperation:fast setting, or
 *        %NULL
 * @delete_mode: return location for the Gsd.SecureDeleteOperation:mode setting,
 *               or %NULL
 * @zeroise: return location for the Gsd.ZeroableOperation:zeroise setting, or
 *           %NULL
 */
static gboolean
operation_confirm_dialog (GtkWindow                    *parent,
                          const gchar                  *primary_text,
                          const gchar                  *secondary_text,
                          const gchar                  *confirm_button_text,
                          gboolean                     *fast,
                          GsdSecureDeleteOperationMode *delete_mode,
                          gboolean                     *zeroise)
{
  GtkResponseType response = GTK_RESPONSE_NONE;
  GtkWidget      *dialog;
  
  dialog = gtk_message_dialog_new (parent,
                                   GTK_DIALOG_DESTROY_WITH_PARENT,
                                   GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE,
                                   "%s", primary_text);
  if (secondary_text) {
    gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
                                              "%s", secondary_text);
  }
  gtk_dialog_add_buttons (GTK_DIALOG (dialog),
                          GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
                          confirm_button_text, GTK_RESPONSE_ACCEPT,
                          NULL);
  /* if we have settings to choose */
  if (fast || delete_mode || zeroise) {
    GtkWidget *content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
    GtkWidget *expander;
    GtkWidget *box;
    
    expander = gtk_expander_new (_("Options"));
    gtk_container_add (GTK_CONTAINER (content_area), expander);
    box = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (expander), box);
    /* delete mode option */
    if (delete_mode) {
      GtkWidget        *hbox;
      GtkWidget        *label;
      GtkWidget        *combo;
      GtkListStore     *store;
      GtkCellRenderer  *renderer;
      
      hbox = gtk_hbox_new (FALSE, 5);
      gtk_box_pack_start (GTK_BOX (box), hbox, FALSE, TRUE, 0);
      label = gtk_label_new (_("Number of passes:"));
      gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
      gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
      /* store columns: setting value     (enum)
       *                number of passes  (int)
       *                descriptive text  (string) */
      store = gtk_list_store_new (3, G_TYPE_INT, G_TYPE_INT, G_TYPE_STRING);
      combo = gtk_combo_box_new_with_model (GTK_TREE_MODEL (store));
      /* number of passes column */
      renderer = gtk_cell_renderer_spin_new ();
      gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), renderer, FALSE);
      gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo), renderer,
                                      "text", 1, NULL);
      /* comment column */
      renderer = gtk_cell_renderer_text_new ();
      gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), renderer, TRUE);
      gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo), renderer,
                                      "text", 2, NULL);
      /* Adds an item.
       * @value: the setting to return if selected
       * @n_pass: the number of pass this setting shows
       * @text: description text for this setting */
      #define ADD_ITEM(value, n_pass, text)                                    \
        G_STMT_START {                                                         \
          GtkTreeIter iter;                                                    \
                                                                               \
          gtk_list_store_append (store, &iter);                                \
          gtk_list_store_set (store, &iter, 0, value, 1, n_pass, 2, text, -1); \
          if (value == *delete_mode) {                                         \
              gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combo), &iter);    \
          }                                                                    \
        } G_STMT_END
      /* add items */
      ADD_ITEM (GSD_SECURE_DELETE_OPERATION_MODE_NORMAL,
                38, _("(secure, recommended)"));
      ADD_ITEM (GSD_SECURE_DELETE_OPERATION_MODE_INSECURE,
                2, _("(insecure, but faster)"));
      ADD_ITEM (GSD_SECURE_DELETE_OPERATION_MODE_VERY_INSECURE,
                1, _("(very insecure, but fastest)"));
      
      #undef ADD_ITEM
      /* connect change & pack */
      g_signal_connect (combo, "changed",
                        G_CALLBACK (pref_enum_combo_changed_handler), delete_mode);
      gtk_box_pack_start (GTK_BOX (hbox), combo, FALSE, TRUE, 0);
    }
    /* fast option */
    if (fast) {
      GtkWidget *check;
      
      check = gtk_check_button_new_with_label (
        _("Fast (and insecure) mode: no /dev/urandom, no synchronize mode")
      );
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check), *fast);
      g_signal_connect (check, "toggled",
                        G_CALLBACK (pref_bool_toggle_changed_handler), fast);
      gtk_box_pack_start (GTK_BOX (box), check, FALSE, TRUE, 0);
    }
    /* "zeroise" option */
    if (zeroise) {
      GtkWidget *check;
      
      check = gtk_check_button_new_with_label (
        _("Wipe the last write with zeros instead of random data")
      );
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check), *zeroise);
      g_signal_connect (check, "toggled",
                        G_CALLBACK (pref_bool_toggle_changed_handler), zeroise);
      gtk_box_pack_start (GTK_BOX (box), check, FALSE, TRUE, 0);
    }
    gtk_widget_show_all (expander);
  }
  /* run the dialog */
  response = gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
  
  return response == GTK_RESPONSE_ACCEPT;
}

static void
progress_dialog_response_handler (GtkDialog *dialog,
                                  gint       response_id,
                                  gpointer   data)
{
  struct NautilusSrmOperationData *opdata = data;
  
  switch (response_id) {
    case GTK_RESPONSE_CANCEL:
    case GTK_RESPONSE_DELETE_EVENT:
      if (display_dialog (GTK_WINDOW (dialog), GTK_MESSAGE_QUESTION, TRUE,
                          _("Are you sure you want to cancel the operation?"),
                          _("Canceling an operation might leave some file(s) in an intermediate state."),
                          _("Continue operation"), GTK_RESPONSE_REJECT,
                          _("Cancel operation"), GTK_RESPONSE_ACCEPT,
                          NULL) == GTK_RESPONSE_ACCEPT) {
        gsd_async_operation_cancel (opdata->operation);
      }
      break;
    
    default:
      break;
  }
}

/* 
 * nautilus_srm_operation_manager_run:
 * @parent: Parent window for dialogs
 * @files: List of #NautilusFileInfo to pass to @operation_launcher_func
 * @confirm_primary_text: Primary text for the confirmation dialog
 * @confirm_secondary_text: Secondary text for the confirmation dialog
 * @confirm_button_text: Text for the confirm button of the confirmation dialog.
 *                       It may be a GTK stock item.
 * @progress_dialog_text: Text for the progress dialog
 * @operation_launcher_func: the function that will be launched to do the operation
 * @failed_primary_text: Primary text of the dialog displayed if operation failed.
 *                       (secondary is the error message)
 * @success_primary_text: Primary text for the the success dialog
 * @success_secondary_text: Secondary text for the the success dialog
 * 
 * 
 */
void
nautilus_srm_operation_manager_run (GtkWindow                *parent,
                                    GList                    *files,
                                    const gchar              *confirm_primary_text,
                                    const gchar              *confirm_secondary_text,
                                    const gchar              *confirm_button_text,
                                    const gchar              *progress_dialog_text,
                                    NautilusSrmOperationFunc  operation_launcher_func,
                                    const gchar              *failed_primary_text,
                                    const gchar              *success_primary_text,
                                    const gchar              *success_secondary_text)
{
  /* if the user confirms, try to launch the operation */
  gboolean                      fast        = FALSE;
  GsdSecureDeleteOperationMode  delete_mode = GSD_SECURE_DELETE_OPERATION_MODE_NORMAL;
  gboolean                      zeroise     = FALSE;
  
  if (operation_confirm_dialog (parent,
                                confirm_primary_text, confirm_secondary_text,
                                confirm_button_text,
                                &fast, &delete_mode, &zeroise)) {
    GError                           *err = NULL;
    struct NautilusSrmOperationData  *opdata;
    
    opdata = g_slice_alloc (sizeof *opdata);
    opdata->window = parent;
    opdata->window_destroy_hid = g_signal_connect (opdata->window, "destroy",
                                                   G_CALLBACK (opdata_window_destroy_handler), opdata);
    opdata->progress_dialog = NAUTILUS_SRM_PROGRESS_DIALOG (nautilus_srm_progress_dialog_new (opdata->window, 0,
                                                                                              progress_dialog_text));
    nautilus_srm_progress_dialog_set_has_cancel_button (opdata->progress_dialog, TRUE);
    g_signal_connect (opdata->progress_dialog, "response",
                      G_CALLBACK (progress_dialog_response_handler), opdata);
    opdata->failed_primary_text = g_strdup (failed_primary_text);
    opdata->success_primary_text = g_strdup (success_primary_text);
    opdata->success_secondary_text = g_strdup (success_secondary_text);
    opdata->operation = operation_launcher_func (files, fast, delete_mode, zeroise,
                                                 G_CALLBACK (operation_finished_handler),
                                                 G_CALLBACK (operation_progress_handler),
                                                 opdata, &err);
    if (! opdata->operation) {
      if (err->code == G_SPAWN_ERROR_NOENT) {
        gchar *message;
        
        /* Merge the error message with our. Pretty much a hack, but should be
         * correct and more precise. */
        message = g_strdup_printf (_("%s. "
                                     "Please make sure you have the secure-delete "
                                     "package properly installed on your system."),
                                   err->message);
        display_operation_error (opdata, message);
        g_free (message);
      } else {
        display_operation_error (opdata, err->message);
      }
      g_error_free (err);
      gtk_widget_destroy (GTK_WIDGET (opdata->progress_dialog));
      free_opdata (opdata);
    } else {
      gtk_widget_show (GTK_WIDGET (opdata->progress_dialog));
    }
  }
}

