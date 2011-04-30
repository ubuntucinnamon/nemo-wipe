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

#ifndef NAUTILUS_WIPE_OPERATION_MANAGER_H
#define NAUTILUS_WIPE_OPERATION_MANAGER_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>
#include <gsecuredelete/gsecuredelete.h>

G_BEGIN_DECLS


/**
 * NautilusWipeOperationFunc:
 * @files: Paths to work on
 * @fast: The Gsd.SecureDeleteOperation:fast setting
 * @mode: The Gsd.SecureDeleteOperation:mode setting
 * @zeroise: The Gsd.ZeroableOperation:zeroise setting
 * @finished_handler: Handler for GsdAsyncOperation::finished
 * @progress_handler: Handler for GsdAsyncOperation::progress
 * @data: User data for @finished_hanlder and @progress_handler
 * @error: Return location for errors, or %NULL to ignore them
 * 
 * 
 * Returns: The operation object that was launched, or %NULL on failure.
 *          The operation object should be unref'd with g_object_unref() when
 *          no longer needed.
 */
typedef GsdAsyncOperation  *(*NautilusWipeOperationFunc)  (GList                       *files,
                                                           gboolean                     fast,
                                                           GsdSecureDeleteOperationMode mode,
                                                           gboolean                     zeroise,
                                                           GCallback                    finished_handler,
                                                           GCallback                    progress_handler,
                                                           gpointer                     data,
                                                           GError                     **error);

void    nautilus_wipe_operation_manager_run   (GtkWindow                *parent,
                                               GList                    *files,
                                               const gchar              *confirm_primary_text,
                                               const gchar              *confirm_secondary_text,
                                               const gchar              *confirm_button_text,
                                               GtkWidget                *confirm_button_icon,
                                               const gchar              *progress_dialog_text,
                                               NautilusWipeOperationFunc operation_launcher_func,
                                               const gchar              *failed_primary_text,
                                               const gchar              *success_primary_text,
                                               const gchar              *success_secondary_text);


G_END_DECLS

#endif /* guard */
