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


/**
 * NautilusSrmFillOperationError:
 * @NAUTILUS_SRM_FILL_OPERATION_ERROR_MISSING_MOUNT: A file have no mount
 * @NAUTILUS_SRM_FILL_OPERATION_ERROR_REMOTE_MOUNT: A mount is not local
 * @NAUTILUS_SRM_FILL_OPERATION_ERROR_FAILED: An error occurred
 * 
 * Possible errors from the %NAUTILUS_SRM_FILL_OPERATION_ERROR domain.
 */
typedef enum
{
  NAUTILUS_SRM_FILL_OPERATION_ERROR_MISSING_MOUNT,
  NAUTILUS_SRM_FILL_OPERATION_ERROR_REMOTE_MOUNT,
  NAUTILUS_SRM_FILL_OPERATION_ERROR_FAILED
} NautilusSrmFillOperationError;

/**
 * NAUTILUS_SRM_FILL_OPERATION_ERROR:
 * 
 * Domain for error coming from a NautilusSrm's fill operation.
 */
#define NAUTILUS_SRM_FILL_OPERATION_ERROR (nautilus_srm_fill_operation_error_quark ())

GQuark   nautilus_srm_fill_operation_error_quark  (void) G_GNUC_CONST;
gboolean nautilus_srm_fill_operation_filter_files (GList    *paths,
                                                   GList   **work_paths_,
                                                   GList   **work_mounts_,
                                                   GError  **error);
GsdAsyncOperation  *nautilus_srm_fill_operation   (GList                       *files,
                                                   gboolean                     fast,
                                                   GsdSecureDeleteOperationMode mode,
                                                   gboolean                     zeroise,
                                                   GCallback                    finished_handler,
                                                   GCallback                    progress_handler,
                                                   gpointer                     data,
                                                   GError                     **error);


G_END_DECLS

#endif /* guard */
