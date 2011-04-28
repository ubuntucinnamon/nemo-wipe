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

#include "progress-dialog.h"

#include <stdarg.h>
#include <glib.h>
#include <gtk/gtk.h>

#include "compat.h" /* for gtk_dialog_get_action_area(),
                     *     gtk_dialog_get_content_area() and
                     *     gtk_widget_get_sensitive() */



struct _NautilusSrmProgressDialogPrivate {
  GtkLabel       *label;
  GtkProgressBar *progress;
  GtkWidget      *cancel_button;
  GtkWidget      *close_button;
  gboolean        finished;
  gboolean        canceled;
  gboolean        auto_hide_action_area;
};

#define GET_PRIVATE(obj)  (G_TYPE_INSTANCE_GET_PRIVATE ((obj), NAUTILUS_TYPE_SRM_PROGRESS_DIALOG, NautilusSrmProgressDialogPrivate))

enum
{
  PROP_0,
  PROP_TEXT,
  PROP_HAS_CANCEL_BUTTON,
  PROP_HAS_CLOSE_BUTTON,
  PROP_AUTO_HIDE_ACTION_AREA
};

G_DEFINE_TYPE (NautilusSrmProgressDialog, nautilus_srm_progress_dialog, GTK_TYPE_DIALOG)

static void
nautilus_srm_progress_dialog_set_property (GObject      *obj,
                                           guint         prop_id,
                                           const GValue *value,
                                           GParamSpec   *pspec)
{
  NautilusSrmProgressDialog *self = NAUTILUS_SRM_PROGRESS_DIALOG (obj);
  
  switch (prop_id) {
    case PROP_TEXT:
      nautilus_srm_progress_dialog_set_text (self, "%s", g_value_get_string (value));
      break;
    
    case PROP_HAS_CANCEL_BUTTON:
      nautilus_srm_progress_dialog_set_has_cancel_button (self, g_value_get_boolean (value));
      break;
    
    case PROP_HAS_CLOSE_BUTTON:
      nautilus_srm_progress_dialog_set_has_close_button (self, g_value_get_boolean (value));
      break;
    
    case PROP_AUTO_HIDE_ACTION_AREA:
      nautilus_srm_progress_dialog_set_auto_hide_action_area (self, g_value_get_boolean (value));
      break;
    
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
  }
}

static void
nautilus_srm_progress_dialog_get_property (GObject    *obj,
                                           guint       prop_id,
                                           GValue     *value,
                                           GParamSpec *pspec)
{
  NautilusSrmProgressDialog *self = NAUTILUS_SRM_PROGRESS_DIALOG (obj);
  
  switch (prop_id) {
    case PROP_TEXT:
      g_value_set_string (value, nautilus_srm_progress_dialog_get_text (self));
      break;
    
    case PROP_HAS_CANCEL_BUTTON:
      g_value_set_boolean (value, nautilus_srm_progress_dialog_get_has_cancel_button (self));
      break;
    
    case PROP_HAS_CLOSE_BUTTON:
      g_value_set_boolean (value, nautilus_srm_progress_dialog_get_has_close_button (self));
      break;
    
    case PROP_AUTO_HIDE_ACTION_AREA:
      g_value_set_boolean (value, nautilus_srm_progress_dialog_get_auto_hide_action_area (self));
      break;
    
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, prop_id, pspec);
  }
}

static void
nautilus_srm_progress_dialog_response (GtkDialog *dialog,
                                       gint       response_id)
{
  NautilusSrmProgressDialog *self = NAUTILUS_SRM_PROGRESS_DIALOG (dialog);
  
  if (GTK_IS_WIDGET (self->priv->cancel_button)) {
    gtk_widget_set_sensitive (self->priv->cancel_button,
                              ! (self->priv->canceled || self->priv->finished));
  }
  if (GTK_IS_WIDGET (self->priv->close_button)) {
    gtk_widget_set_sensitive (self->priv->close_button,
                              self->priv->finished || self->priv->canceled);
  }
  
  if (GTK_DIALOG_CLASS (nautilus_srm_progress_dialog_parent_class)->response) {
    GTK_DIALOG_CLASS (nautilus_srm_progress_dialog_parent_class)->response (dialog, response_id);
  }
}

static void
update_action_area_visibility (NautilusSrmProgressDialog *dialog,
                               gboolean                   force_show)
{
  if (dialog->priv->auto_hide_action_area || force_show) {
    GtkWidget  *container;
    guint       n_children = 0;
    
    container = gtk_dialog_get_action_area (GTK_DIALOG (dialog));
    if (force_show) {
      n_children = 1;
    } else {
      GList *children;
      
      children = gtk_container_get_children (GTK_CONTAINER (container));
      n_children = g_list_length (children);
      g_list_free (children);
    }
    
    gtk_dialog_set_has_separator (GTK_DIALOG (dialog), n_children > 0);
    if (n_children > 0) {
      gtk_widget_show (container);
    } else {
      gtk_widget_hide (container);
    }
  }
}

static void
nautilus_srm_progress_dialog_init (NautilusSrmProgressDialog *self)
{
  GtkWidget *vbox;
  
  self->priv = GET_PRIVATE (self);
  self->priv->progress = GTK_PROGRESS_BAR (gtk_progress_bar_new ());
  self->priv->label = GTK_LABEL (gtk_label_new (""));
  self->priv->close_button = NULL;
  self->priv->cancel_button = NULL;
  self->priv->finished = FALSE;
  self->priv->canceled = FALSE;
  self->priv->auto_hide_action_area = FALSE;
  
  vbox = gtk_dialog_get_content_area (GTK_DIALOG (self));
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 10);
  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (self->priv->label), TRUE, TRUE, 7);
  gtk_widget_show (GTK_WIDGET (self->priv->label));
  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (self->priv->progress), FALSE, TRUE, 7);
  gtk_widget_show (GTK_WIDGET (self->priv->progress));
  
  gtk_progress_bar_set_ellipsize (self->priv->progress, PANGO_ELLIPSIZE_END);
  update_action_area_visibility (self, FALSE);
}

static void
nautilus_srm_progress_dialog_finalize (GObject *obj)
{
  NautilusSrmProgressDialog *self = NAUTILUS_SRM_PROGRESS_DIALOG (obj);
  
  G_OBJECT_CLASS (nautilus_srm_progress_dialog_parent_class)->finalize (obj);
}

static gboolean
nautilus_srm_progress_dialog_delete_event (GtkWidget *widget,
                                           /* The doc says GdkEvent but it is
                                            * actually GdkEventAny. Nothing
                                            * bad as it is a sort of "base
                                            * class" for all other events.
                                            * See the doc. */
                                           GdkEventAny  *event)
{
  return TRUE;
}

static void
nautilus_srm_progress_dialog_class_init (NautilusSrmProgressDialogClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkDialogClass *dialog_class = GTK_DIALOG_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  
  object_class->set_property = nautilus_srm_progress_dialog_set_property;
  object_class->get_property = nautilus_srm_progress_dialog_get_property;
  object_class->finalize     = nautilus_srm_progress_dialog_finalize;
  dialog_class->response     = nautilus_srm_progress_dialog_response;
  /* our default handler prevent the dialog to be destroyed if the user tries to
   * close the dialog. This doesn't prevent the DELETE_EVENT response to be
   * triggered */
  widget_class->delete_event = nautilus_srm_progress_dialog_delete_event;
  
  g_object_class_install_property (object_class, PROP_TEXT,
                                   g_param_spec_string ("text",
                                                        "Text",
                                                        "Text of the dialog",
                                                        "",
                                                        G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_HAS_CANCEL_BUTTON,
                                   g_param_spec_boolean ("has-cancel-button",
                                                         "Has cancel button",
                                                         "Whether the dialog has a cancel button",
                                                         FALSE,
                                                         G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_HAS_CLOSE_BUTTON,
                                   g_param_spec_boolean ("has-close-button",
                                                         "Has close button",
                                                         "Whether the dialog has a close button",
                                                         FALSE,
                                                         G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_HAS_CLOSE_BUTTON,
                                   g_param_spec_boolean ("auto-hide-action-area",
                                                         "Auto hide action area",
                                                         "Whether the action area should be hidden automatically "
                                                         "if there is no action widget. This option may have "
                                                         "unexpected behavior if used together with "
                                                         "GtkDialog:has-separator.",
                                                         FALSE,
                                                         G_PARAM_READWRITE));
  
  g_type_class_add_private (klass, sizeof (NautilusSrmProgressDialogPrivate));
}


/**
 * nautilus_srm_progress_dialog_new:
 * @parent: The parent window for the dialog, or %NULL for none
 * @flags: Some #GtkDialogFlags or 0
 * @format: format for the dialog's text
 * @...: printf-like argument for @format.
 * 
 * Creates a new NautilusSrmProgressDialog.
 * For the @format and @... arguments, see
 * nautilus_srm_progress_dialog_set_text().
 *
 * Returns: The newly created dialog.
 */
GtkWidget *
nautilus_srm_progress_dialog_new (GtkWindow       *parent,
                                  GtkDialogFlags   flags,
                                  const gchar     *format,
                                  ...)
{
  GtkWidget  *self;
  gchar      *text;
  va_list     ap;
  
  va_start (ap, format);
  text = g_strdup_vprintf (format, ap);
  va_end (ap);
  self = g_object_new (NAUTILUS_TYPE_SRM_PROGRESS_DIALOG,
                       "transient-for",       parent,
                       "has-separator",       ! (flags & GTK_DIALOG_NO_SEPARATOR),
                       "modal",               flags & GTK_DIALOG_MODAL,
                       "destroy-with-parent", flags & GTK_DIALOG_DESTROY_WITH_PARENT,
                       "text",                text,
                       NULL);
  g_free (text);
  update_action_area_visibility (NAUTILUS_SRM_PROGRESS_DIALOG (self), FALSE);
  
  return self;
}

/**
 * nautilus_srm_progress_dialog_set_fraction:
 * @dialog: A #NautilusSrmProgressDialog
 * @fraction: The current progression.
 * 
 * See gtk_progress_bar_set_fraction().
 */
void
nautilus_srm_progress_dialog_set_fraction (NautilusSrmProgressDialog *dialog,
                                           gdouble                    fraction)
{
  g_return_if_fail (NAUTILUS_IS_SRM_PROGRESS_DIALOG (dialog));
  
  gtk_progress_bar_set_fraction (dialog->priv->progress, fraction);
}

/**
 * nautilus_srm_progress_dialog_get_fraction:
 * @dialog: A #NautilusSrmProgressDialog
 * 
 * See gtk_progress_bar_get_fraction().
 * 
 * Returns: The current progress of the dialog's progress bar.
 */
gdouble
nautilus_srm_progress_dialog_get_fraction (NautilusSrmProgressDialog *dialog)
{
  g_return_val_if_fail (NAUTILUS_IS_SRM_PROGRESS_DIALOG (dialog), 0.0);
  
  return gtk_progress_bar_get_fraction (dialog->priv->progress);
}

/**
 * nautilus_srm_progress_dialog_pulse:
 * @dialog: A #NautilusSrmProgressDialog
 * 
 * See gtk_progress_bar_pulse().
 */
void
nautilus_srm_progress_dialog_pulse (NautilusSrmProgressDialog *dialog)
{
  g_return_if_fail (NAUTILUS_IS_SRM_PROGRESS_DIALOG (dialog));
  
  gtk_progress_bar_pulse (dialog->priv->progress);
}

/**
 * nautilus_srm_progress_dialog_set_pulse_step:
 * @dialog: A #NautilusSrmProgressDialog
 * @fraction: The pulse step of the dialog's progress bar.
 * 
 * See gtk_progress_bar_set_pulse_step().
 */
void
nautilus_srm_progress_dialog_set_pulse_step (NautilusSrmProgressDialog *dialog,
                                             gdouble                    fraction)
{
  g_return_if_fail (NAUTILUS_IS_SRM_PROGRESS_DIALOG (dialog));
  
  gtk_progress_bar_set_pulse_step (dialog->priv->progress, fraction);
}

/**
 * nautilus_srm_progress_dialog_get_pulse_step:
 * @dialog: A #NautilusSrmProgressDialog
 * 
 * See gtk_progress_bar_get_pulse_step().
 * 
 * Returns: The progress step of the dialog's progress bar.
 */
gdouble
nautilus_srm_progress_dialog_get_pulse_step (NautilusSrmProgressDialog *dialog)
{
  g_return_val_if_fail (NAUTILUS_IS_SRM_PROGRESS_DIALOG (dialog), 0.0);
  
  return gtk_progress_bar_get_pulse_step (dialog->priv->progress);
}

/**
 * nautilus_srm_progress_dialog_set_progress_text:
 * @dialog: A #NautilusSrmProgressDialog
 * @format: Text format (printf-like)
 * @...: Arguments for @format
 * 
 * Sets the progress text. For details about @format and @..., see the
 * documentation of g_strdup_printf().
 * Don't mistake this function for nautilus_srm_progress_dialog_set_text().
 */
void
nautilus_srm_progress_dialog_set_progress_text (NautilusSrmProgressDialog *dialog,
                                                const gchar               *format,
                                                ...)
{
  gchar  *text;
  va_list ap;
  
  g_return_if_fail (NAUTILUS_IS_SRM_PROGRESS_DIALOG (dialog));
  
  va_start (ap, format);
  text = g_strdup_vprintf (format, ap);
  va_end (ap);
  gtk_progress_bar_set_text (dialog->priv->progress, text);
  g_free (text);
}

/**
 * nautilus_srm_progress_dialog_get_progress_text:
 * @dialog: A #NautilusSrmProgressDialog
 * 
 * Gets the current progress text of @dialog. Don't mistake for
 * nautilus_srm_progress_dialog_get_text().
 * 
 * Returns: The progress text of the dialog.
 */
const gchar *
nautilus_srm_progress_dialog_get_progress_text (NautilusSrmProgressDialog *dialog)
{
  g_return_val_if_fail (NAUTILUS_IS_SRM_PROGRESS_DIALOG (dialog), NULL);
  
  return gtk_progress_bar_get_text (dialog->priv->progress);
}

/**
 * nautilus_srm_progress_dialog_set_text:
 * @dialog: A #NautilusSrmProgressDialog
 * @format: Text format (printf-like)
 * @...: Arguments for @format
 * 
 * Sets the dialog's text. For details about @format and @..., see the
 * documentation of g_strdup_printf().
 * Don't mistake this function for
 * nautilus_srm_progress_dialog_set_progress_text() that do the same but for the
 * progress text instead of the dialog's main text.
 */
void
nautilus_srm_progress_dialog_set_text (NautilusSrmProgressDialog *dialog,
                                       const gchar               *format,
                                       ...)
{
  gchar  *text;
  va_list ap;
  
  g_return_if_fail (NAUTILUS_IS_SRM_PROGRESS_DIALOG (dialog));
  
  va_start (ap, format);
  text = g_strdup_vprintf (format, ap);
  va_end (ap);
  gtk_label_set_text (dialog->priv->label, text);
  g_free (text);
}

/**
 * nautilus_srm_progress_dialog_get_text:
 * @dialog: A #NautilusSrmProgressDialog
 * 
 * Gets the text message of the dialog. Don't mistake this function for
 * nautilus_srm_progress_dialog_get_progress_text().
 * 
 * Returns: The text of @dialog.
 */
const gchar *
nautilus_srm_progress_dialog_get_text (NautilusSrmProgressDialog *dialog)
{
  g_return_val_if_fail (NAUTILUS_IS_SRM_PROGRESS_DIALOG (dialog), NULL);
  
  return gtk_label_get_text (dialog->priv->label);
}

void
nautilus_srm_progress_dialog_cancel (NautilusSrmProgressDialog *dialog)
{
  g_return_if_fail (NAUTILUS_IS_SRM_PROGRESS_DIALOG (dialog));
  
  if (! dialog->priv->canceled) {
    dialog->priv->canceled = TRUE;
    gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog), GTK_RESPONSE_CANCEL,
                                       dialog->priv->canceled);
    gtk_dialog_response (GTK_DIALOG (dialog), GTK_RESPONSE_CANCEL);
  }
}

gboolean
nautilus_srm_progress_dialog_is_canceled (NautilusSrmProgressDialog *dialog)
{
  g_return_val_if_fail (NAUTILUS_IS_SRM_PROGRESS_DIALOG (dialog), FALSE);
  
  return dialog->priv->canceled;
}

/**
 * nautilus_srm_progress_dialog_finish:
 * @dialog: A #NautilusSrmProgressDialog
 * @success: Whether the operation finished successfully or not
 * 
 * 
 */
void
nautilus_srm_progress_dialog_finish (NautilusSrmProgressDialog *dialog,
                                     gboolean                   success)
{
  g_return_if_fail (NAUTILUS_IS_SRM_PROGRESS_DIALOG (dialog));
  
  dialog->priv->finished = TRUE;
  if (success) {
    /* ensure the progression is shown completed */
    nautilus_srm_progress_dialog_set_fraction (dialog, 1.0);
  }
  gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog), GTK_RESPONSE_CANCEL,
                                     FALSE);
  gtk_dialog_response (GTK_DIALOG (dialog),
                       NAUTILUS_SRM_PROGRESS_DIALOG_RESPONSE_COMPLETE);
}

/**
 * nautilus_srm_progress_dialog_is_finished:
 * @dialog: A #NautilusSrmProgressDialog
 * 
 * Gets whether the operation that @dialog displays is finished or not.
 * This is set by nautilus_srm_progress_dialog_finish().
 * 
 * Returns: Whether the operation displayed by @dialog is finished or not.
 */
gboolean
nautilus_srm_progress_dialog_is_finished (NautilusSrmProgressDialog *dialog)
{
  g_return_val_if_fail (NAUTILUS_IS_SRM_PROGRESS_DIALOG (dialog), FALSE);
  
  return dialog->priv->finished;
}

/**
 * nautilus_srm_progress_dialog_set_has_close_button:
 * @dialog: A #NautilusSrmProgressDialog
 * @has_close_button: Whether the dialog should have a close button.
 * 
 * Sets whether the dialog has a close button. Enabling close button at the
 * progress dialog level enable automatic sensitivity update of the button
 * according to the current operation state (sensitive only if the operation is
 * either finished or canceled).
 */
void
nautilus_srm_progress_dialog_set_has_close_button (NautilusSrmProgressDialog *dialog,
                                                   gboolean                   has_close_button)
{
  g_return_if_fail (NAUTILUS_IS_SRM_PROGRESS_DIALOG (dialog));
  
  if (has_close_button != (dialog->priv->close_button != NULL)) {
    if (has_close_button) {
      dialog->priv->close_button = gtk_dialog_add_button (GTK_DIALOG (dialog),
                                                          GTK_STOCK_CLOSE,
                                                          GTK_RESPONSE_CLOSE);
      gtk_widget_set_sensitive (dialog->priv->close_button,
                                dialog->priv->finished || dialog->priv->canceled);
    } else {
      gtk_widget_destroy (dialog->priv->close_button);
      dialog->priv->close_button = NULL;
    }
    update_action_area_visibility (dialog, FALSE);
  }
}

/**
 * nautilus_srm_progress_dialog_get_has_close_button:
 * @dialog: A #NautilusSrmProgressDialog
 * 
 * Returns: Whether @dialog has a close button or not.
 */
gboolean
nautilus_srm_progress_dialog_get_has_close_button (NautilusSrmProgressDialog *dialog)
{
  g_return_val_if_fail (NAUTILUS_IS_SRM_PROGRESS_DIALOG (dialog), FALSE);
  
  return dialog->priv->close_button != NULL;
}

/**
 * nautilus_srm_progress_dialog_set_has_cancel_button:
 * @dialog: A #NautilusSrmProgressDialog
 * @has_cancel_button: Whether the dialog should have a cancel button.
 * 
 * Sets whether the dialog has a cancel button. Enabling cancel button at the
 * progress dialog level enable automatic sensitivity update of the button
 * according to the current operation state (sensitive only if the operation is
 * neither finished nor canceled).
 */
void
nautilus_srm_progress_dialog_set_has_cancel_button (NautilusSrmProgressDialog *dialog,
                                                    gboolean                   has_cancel_button)
{
  g_return_if_fail (NAUTILUS_IS_SRM_PROGRESS_DIALOG (dialog));
  
  if (has_cancel_button != (dialog->priv->cancel_button != NULL)) {
    if (has_cancel_button) {
      dialog->priv->cancel_button = gtk_dialog_add_button (GTK_DIALOG (dialog),
                                                           GTK_STOCK_CANCEL,
                                                           GTK_RESPONSE_CANCEL);
      gtk_widget_set_sensitive (dialog->priv->cancel_button,
                                ! dialog->priv->canceled && ! dialog->priv->finished);
    } else {
      gtk_widget_destroy (dialog->priv->cancel_button);
      dialog->priv->cancel_button = NULL;
    }
    update_action_area_visibility (dialog, FALSE);
  }
}

/**
 * nautilus_srm_progress_dialog_get_has_cancel_button:
 * @dialog: A #NautilusSrmProgressDialog
 * 
 * Returns: Whether @dialog has a cancel button.
 */
gboolean
nautilus_srm_progress_dialog_get_has_cancel_button (NautilusSrmProgressDialog *dialog)
{
  g_return_val_if_fail (NAUTILUS_IS_SRM_PROGRESS_DIALOG (dialog), FALSE);
  
  return dialog->priv->cancel_button != NULL;
}

void
nautilus_srm_progress_dialog_set_auto_hide_action_area (NautilusSrmProgressDialog *dialog,
                                                        gboolean                   auto_hide)
{
  g_return_if_fail (NAUTILUS_IS_SRM_PROGRESS_DIALOG (dialog));
  
  if (auto_hide != dialog->priv->auto_hide_action_area) {
    dialog->priv->auto_hide_action_area = auto_hide;
    update_action_area_visibility (dialog, auto_hide == FALSE);
  }
}

gboolean
nautilus_srm_progress_dialog_get_auto_hide_action_area (NautilusSrmProgressDialog *dialog)
{
  g_return_val_if_fail (NAUTILUS_IS_SRM_PROGRESS_DIALOG (dialog), FALSE);
  
  return dialog->priv->auto_hide_action_area;
}
