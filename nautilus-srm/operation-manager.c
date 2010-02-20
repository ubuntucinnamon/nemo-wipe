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

#include "progress-dialog.h"


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
                                   GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
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
  g_signal_connect (dialog, "response", G_CALLBACK (gtk_widget_destroy), NULL);
  if (wait_for_response) {
    response = gtk_dialog_run (GTK_DIALOG (dialog));
  } else {
    gtk_widget_show (dialog);
  }
  
  return response;
}



struct NautilusSrmOperationData
{
  GtkWindow                  *window;
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
  g_free (opdata->failed_primary_text);
  g_free (opdata->success_primary_text);
  g_free (opdata->success_secondary_text);
  g_slice_free1 (sizeof *opdata, opdata);
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
  if (display_dialog (parent, GTK_MESSAGE_QUESTION, TRUE,
                      confirm_primary_text, confirm_secondary_text,
                      GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
                      confirm_button_text, GTK_RESPONSE_ACCEPT,
                      NULL) == GTK_RESPONSE_ACCEPT) {
    GError                           *err = NULL;
    struct NautilusSrmOperationData  *opdata;
    
    opdata = g_slice_alloc (sizeof *opdata);
    opdata->window = parent;
    opdata->progress_dialog = NAUTILUS_SRM_PROGRESS_DIALOG (nautilus_srm_progress_dialog_new (opdata->window, 0,
                                                                                              progress_dialog_text));
    opdata->failed_primary_text = g_strdup (failed_primary_text);
    opdata->success_primary_text = g_strdup (success_primary_text);
    opdata->success_secondary_text = g_strdup (success_secondary_text);
    if (! operation_launcher_func (files,
                                   G_CALLBACK (operation_finished_handler),
                                   G_CALLBACK (operation_progress_handler),
                                   opdata, &err)) {
      display_operation_error (opdata, err->message);
      g_error_free (err);
      gtk_widget_destroy (GTK_WIDGET (opdata->progress_dialog));
      free_opdata (opdata);
    } else {
      gtk_widget_show (GTK_WIDGET (opdata->progress_dialog));
    }
  }
}

