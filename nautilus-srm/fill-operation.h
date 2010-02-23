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

#ifndef NAUTILUS_SRM_FILL_OPERATION_H
#define NAUTILUS_SRM_FILL_OPERATION_H

#include <glib.h>
#include <glib-object.h>
#include <gsecuredelete/gsecuredelete.h>

G_BEGIN_DECLS


gboolean    nautilus_srm_fill_operation     (GList                       *files,
                                             gboolean                     fast,
                                             GsdSecureDeleteOperationMode mode,
                                             gboolean                     zeroise,
                                             GCallback                    finished_handler,
                                             GCallback                    progress_handler,
                                             gpointer                     data,
                                             GError                     **error);


G_END_DECLS

#endif /* guard */
