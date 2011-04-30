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

#ifndef NAUTILUS_WIPE_PROGRESS_DIALOG_H
#define NAUTILUS_WIPE_PROGRESS_DIALOG_H

#include <stdarg.h>
#include <glib.h>
#include <gtk/gtk.h>


G_BEGIN_DECLS


#define NAUTILUS_TYPE_WIPE_PROGRESS_DIALOG          (nautilus_wipe_progress_dialog_get_type ())
#define NAUTILUS_WIPE_PROGRESS_DIALOG(o)            (G_TYPE_CHECK_INSTANCE_CAST ((o), NAUTILUS_TYPE_WIPE_PROGRESS_DIALOG, NautilusWipeProgressDialog))
#define NAUTILUS_WIPE_PROGRESS_DIALOG_CLASS(k)      (G_TYPE_CHECK_CLASS_CAST ((k), NAUTILUS_TYPE_WIPE_PROGRESS_DIALOG, NautilusWipeProgressDialogClass))
#define NAUTILUS_IS_WIPE_PROGRESS_DIALOG(o)         (G_TYPE_CHECK_INSTANCE_TYPE ((o), NAUTILUS_TYPE_WIPE_PROGRESS_DIALOG))
#define NAUTILUS_IS_WIPE_PROGRESS_DIALOG_CLASS(k)   (G_TYPE_CHECK_CLASS_TYPE ((k), NAUTILUS_TYPE_WIPE_PROGRESS_DIALOG))
#define NAUTILUS_WIPE_PROGRESS_DIALOG_GET_CLASS(o)  (G_TYPE_INSTANCE_GET_CLASS ((o), NAUTILUS_TYPE_WIPE_PROGRESS_DIALOG, NautilusWipeProgressDialogClass))

typedef struct _NautilusWipeProgressDialog        NautilusWipeProgressDialog;
typedef struct _NautilusWipeProgressDialogClass   NautilusWipeProgressDialogClass;
typedef struct _NautilusWipeProgressDialogPrivate NautilusWipeProgressDialogPrivate;

struct _NautilusWipeProgressDialog {
  GtkDialog parent_instance;
  NautilusWipeProgressDialogPrivate *priv;
};

struct _NautilusWipeProgressDialogClass {
  GtkDialogClass parent_class;
};

#define NAUTILUS_WIPE_PROGRESS_DIALOG_RESPONSE_COMPLETE 1


GType         nautilus_wipe_progress_dialog_get_type                  (void) G_GNUC_CONST;

GtkWidget    *nautilus_wipe_progress_dialog_new                       (GtkWindow       *parent,
                                                                       GtkDialogFlags   flags,
                                                                       const gchar     *format,
                                                                       ...);
void          nautilus_wipe_progress_dialog_set_fraction              (NautilusWipeProgressDialog  *dialog,
                                                                       gdouble                      fraction);
gdouble       nautilus_wipe_progress_dialog_get_fraction              (NautilusWipeProgressDialog  *dialog);
void          nautilus_wipe_progress_dialog_pulse                     (NautilusWipeProgressDialog  *dialog);
void          nautilus_wipe_progress_dialog_set_pulse_step            (NautilusWipeProgressDialog  *dialog,
                                                                       gdouble                      fraction);
gdouble       nautilus_wipe_progress_dialog_get_pulse_step            (NautilusWipeProgressDialog  *dialog);
void          nautilus_wipe_progress_dialog_set_progress_text         (NautilusWipeProgressDialog  *dialog,
                                                                       const gchar                 *format,
                                                                       ...);
const gchar  *nautilus_wipe_progress_dialog_get_progress_text         (NautilusWipeProgressDialog  *dialog);
void          nautilus_wipe_progress_dialog_set_text                  (NautilusWipeProgressDialog  *dialog,
                                                                       const gchar                 *format,
                                                                       ...);
const gchar  *nautilus_wipe_progress_dialog_get_text                  (NautilusWipeProgressDialog  *dialog);
void          nautilus_wipe_progress_dialog_cancel                    (NautilusWipeProgressDialog  *dialog);
gboolean      nautilus_wipe_progress_dialog_is_canceled               (NautilusWipeProgressDialog  *dialog);
void          nautilus_wipe_progress_dialog_finish                    (NautilusWipeProgressDialog  *dialog,
                                                                       gboolean                     success);
gboolean      nautilus_wipe_progress_dialog_is_finished               (NautilusWipeProgressDialog  *dialog);
void          nautilus_wipe_progress_dialog_set_has_close_button      (NautilusWipeProgressDialog  *dialog,
                                                                       gboolean                     has_close_button);
gboolean      nautilus_wipe_progress_dialog_get_has_close_button      (NautilusWipeProgressDialog  *dialog);
void          nautilus_wipe_progress_dialog_set_has_cancel_button     (NautilusWipeProgressDialog  *dialog,
                                                                       gboolean                     has_close_button);
gboolean      nautilus_wipe_progress_dialog_get_has_cancel_button     (NautilusWipeProgressDialog  *dialog);

void          nautilus_wipe_progress_dialog_set_auto_hide_action_area (NautilusWipeProgressDialog  *dialog,
                                                                       gboolean                     auto_hide);
gboolean      nautilus_wipe_progress_dialog_get_auto_hide_action_area (NautilusWipeProgressDialog  *dialog);


G_END_DECLS

#endif /* guard */
