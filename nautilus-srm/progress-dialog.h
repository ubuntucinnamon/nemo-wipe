/*
 *  nautilus-srm - a nautilus extension to wipe file(s) with srm
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

#ifndef NAUTILUS_SRM_PROGRESS_DIALOG_H
#define NAUTILUS_SRM_PROGRESS_DIALOG_H

#include <stdarg.h>
#include <glib.h>
#include <gtk/gtk.h>


G_BEGIN_DECLS


#define NAUTILUS_TYPE_SRM_PROGRESS_DIALOG         (nautilus_srm_progress_dialog_get_type ())
#define NAUTILUS_SRM_PROGRESS_DIALOG(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), NAUTILUS_TYPE_SRM_PROGRESS_DIALOG, NautilusSrmProgressDialog))
#define NAUTILUS_SRM_PROGRESS_DIALOG_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), NAUTILUS_TYPE_SRM_PROGRESS_DIALOG, NautilusSrmProgressDialogClass))
#define NAUTILUS_IS_SRM_PROGRESS_DIALOG(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), NAUTILUS_TYPE_SRM_PROGRESS_DIALOG))
#define NAUTILUS_IS_SRM_PROGRESS_DIALOG_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), NAUTILUS_TYPE_SRM_PROGRESS_DIALOG))
#define NAUTILUS_SRM_PROGRESS_DIALOG_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), NAUTILUS_TYPE_SRM_PROGRESS_DIALOG, NautilusSrmProgressDialogClass))

typedef struct _NautilusSrmProgressDialog         NautilusSrmProgressDialog;
typedef struct _NautilusSrmProgressDialogClass    NautilusSrmProgressDialogClass;
typedef struct _NautilusSrmProgressDialogPrivate  NautilusSrmProgressDialogPrivate;

struct _NautilusSrmProgressDialog {
  GtkDialog parent_instance;
  NautilusSrmProgressDialogPrivate *priv;
};

struct _NautilusSrmProgressDialogClass {
  GtkDialogClass parent_class;
};

#define NAUTILUS_SRM_PROGRESS_DIALOG_RESPONSE_COMPLETE 1


GType       nautilus_srm_progress_dialog_get_type       (void) G_GNUC_CONST;

GtkWidget  *nautilus_srm_progress_dialog_new            (GtkWindow       *parent,
                                                         GtkDialogFlags   flags,
                                                         const gchar     *format,
                                                         ...);
void        nautilus_srm_progress_dialog_set_fraction   (NautilusSrmProgressDialog *dialog,
                                                         gdouble                    fraction);
gdouble     nautilus_srm_progress_dialog_get_fraction   (NautilusSrmProgressDialog *dialog);
void        nautilus_srm_progress_dialog_pulse          (NautilusSrmProgressDialog *dialog);
void        nautilus_srm_progress_dialog_set_pulse_step (NautilusSrmProgressDialog *dialog,
                                                         gdouble                    fraction);
gdouble     nautilus_srm_progress_dialog_get_pulse_step (NautilusSrmProgressDialog *dialog);
void        nautilus_srm_progress_dialog_set_progress_text
                                                        (NautilusSrmProgressDialog *dialog,
                                                         const gchar               *format,
                                                         ...);
const gchar *
            nautilus_srm_progress_dialog_get_progress_text
                                                        (NautilusSrmProgressDialog *dialog);
void        nautilus_srm_progress_dialog_set_text       (NautilusSrmProgressDialog *dialog,
                                                         const gchar               *format,
                                                         ...);
const gchar *
            nautilus_srm_progress_dialog_get_text       (NautilusSrmProgressDialog *dialog);
void        nautilus_srm_progress_dialog_cancel         (NautilusSrmProgressDialog *dialog);
gboolean    nautilus_srm_progress_dialog_is_canceled    (NautilusSrmProgressDialog *dialog);
void        nautilus_srm_progress_dialog_finish         (NautilusSrmProgressDialog *dialog,
                                                         gboolean                   success);
gboolean    nautilus_srm_progress_dialog_is_finished    (NautilusSrmProgressDialog *dialog);
void        nautilus_srm_progress_dialog_set_has_close_button
                                                        (NautilusSrmProgressDialog *dialog,
                                                         gboolean                   has_close_button);
gboolean    nautilus_srm_progress_dialog_get_has_close_button
                                                        (NautilusSrmProgressDialog *dialog);
void        nautilus_srm_progress_dialog_set_has_cancel_button
                                                        (NautilusSrmProgressDialog *dialog,
                                                         gboolean                   has_close_button);
gboolean    nautilus_srm_progress_dialog_get_has_cancel_button
                                                        (NautilusSrmProgressDialog *dialog);

void        nautilus_srm_progress_dialog_set_auto_hide_action_area
                                                        (NautilusSrmProgressDialog *dialog,
                                                         gboolean                   auto_hide);
gboolean    nautilus_srm_progress_dialog_get_auto_hide_action_area
                                                        (NautilusSrmProgressDialog *dialog);


G_END_DECLS

#endif /* guard */
