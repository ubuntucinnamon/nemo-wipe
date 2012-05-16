/*
 *  nautilus-wipe - a nautilus extension to wipe file(s)
 * 
 *  Copyright (C) 2009-2012 Colomban Wendling <ban@herbesfolles.org>
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "nautilus-wipe.h"

#include <libnautilus-extension/nautilus-menu-provider.h>
#include <libnautilus-extension/nautilus-file-info.h>

#include <glib.h>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include <gio/gio.h>

#include <gsecuredelete/gsecuredelete.h>

#include "path-list.h"
#include "operation-manager.h"
#include "delete-operation.h"
#include "fill-operation.h"
#include "compat.h"
#include "type-utils.h"


/* private prototypes */
static GList *nautilus_wipe_real_get_file_items       (NautilusMenuProvider *provider,
                                                       GtkWidget            *window,
                                                       GList                *files);
static GList *nautilus_wipe_real_get_background_items (NautilusMenuProvider *provider,
                                                       GtkWidget            *window,
                                                       NautilusFileInfo     *current_folder);
static void   nautilus_wipe_menu_provider_iface_init  (NautilusMenuProviderIface *iface);


#define ITEM_DATA_MOUNTPOINTS_KEY "NautilusWipe::mountpoints"
#define ITEM_DATA_PATHS_KEY       "NautilusWipe::paths"
#define ITEM_DATA_WINDOW_KEY      "NautilusWipe::parent-window"



GQuark
nautilus_wipe_error_quark (void)
{
  static GQuark error_quark = 0;
  
  if (G_UNLIKELY (error_quark == 0)) {
    error_quark = g_quark_from_static_string ("NautilusWipeError");
  }
  
  return error_quark;
}

NAUTILUS_WIPE_DEFINE_TYPE_MODULE_WITH_CODE (NautilusWipe,
                                            nautilus_wipe,
                                            G_TYPE_OBJECT,
                                            NAUTILUS_WIPE_TYPE_MODULE_IMPLEMENT_INTERFACE (NAUTILUS_TYPE_MENU_PROVIDER,
                                                                                           nautilus_wipe_menu_provider_iface_init))



static void
nautilus_wipe_menu_provider_iface_init (NautilusMenuProviderIface *iface)
{
  iface->get_file_items       = nautilus_wipe_real_get_file_items;
  iface->get_background_items = nautilus_wipe_real_get_background_items;
}

static void
nautilus_wipe_class_init (NautilusWipeClass *class)
{
}

static void
nautilus_wipe_init (NautilusWipe *self)
{
}

/* Runs the wipe operation */
static void
nautilus_wipe_run_delete_operation (GtkWindow *parent,
                                    GList     *files)
{
  gchar  *confirm_primary_text = NULL;
  guint   n_items;
  
  n_items = g_list_length (files);
  /* FIXME: can't truly use g_dngettext since the args are not the same */
  if (n_items > 1) {
    confirm_primary_text = g_strdup_printf (_("Are you sure you want to wipe "
                                              "the %u selected items?"), n_items);
  } else if (n_items > 0) {
    gchar *name;
    
    name = g_filename_display_basename (files->data);
    confirm_primary_text = g_strdup_printf (_("Are you sure you want to wipe "
                                              "\"%s\"?"),
                                            name);
    g_free (name);
  }
  nautilus_wipe_operation_manager_run (
    parent, files,
    /* confirm dialog */
    confirm_primary_text,
    _("If you wipe an item, it will not be recoverable."),
    _("_Wipe"),
    gtk_image_new_from_stock (GTK_STOCK_DELETE, GTK_ICON_SIZE_BUTTON),
    /* progress dialog */
    _("Wiping files..."),
    /* operation launcher */
    nautilus_wipe_delete_operation,
    /* failed dialog */
    _("Wipe failed."),
    /* success dialog */
    _("Wipe successful."),
    _("Item(s) have been successfully wiped.")
  );
  g_free (confirm_primary_text);
}

/* Runs the fill operation */
static void
nautilus_wipe_run_fill_operation (GtkWindow *parent,
                                  GList     *paths,
                                  GList     *mountpoints)
{
  gchar  *confirm_primary_text = NULL;
  gchar  *success_secondary_text = NULL;
  guint   n_items;
  
  n_items = g_list_length (mountpoints);
  /* FIXME: can't truly use g_dngettext since the args are not the same */
  if (n_items > 1) {
    GList    *tmp;
    GString  *devices = g_string_new (NULL);
    
    for (tmp = mountpoints; tmp; tmp = g_list_next (tmp)) {
      gchar *name;
      
      name = g_filename_display_name (tmp->data);
      if (devices->len > 0) {
        if (! tmp->next) {
          /* TRANSLATORS: this is the last device names separator */
          g_string_append (devices, _(" and "));
        } else {
          /* TRANSLATORS: this is the device names separator (except last) */
          g_string_append (devices, _(", "));
        }
      }
      /* TRANSLATORS: this is the device name */
      g_string_append_printf (devices, _("\"%s\""), name);
      g_free (name);
    }
    confirm_primary_text = g_strdup_printf (_("Are you sure you want to wipe "
                                              "the available diskspace on the "
                                              "%s partitions or devices?"),
                                            devices->str);
    success_secondary_text = g_strdup_printf (_("Available diskspace on the "
                                                "partitions or devices %s "
                                                "have been successfully wiped."),
                                              devices->str);
    g_string_free (devices, TRUE);
  } else if (n_items > 0) {
    gchar *name;
    
    name = g_filename_display_name (mountpoints->data);
    confirm_primary_text = g_strdup_printf (_("Are you sure you want to wipe "
                                              "the available diskspace on the "
                                              "\"%s\" partition or device?"),
                                            name);
    success_secondary_text = g_strdup_printf (_("Available diskspace on the "
                                                "partition or device \"%s\" "
                                                "have been successfully wiped."),
                                              name);
    g_free (name);
  }
  nautilus_wipe_operation_manager_run (
    parent, paths,
    /* confirm dialog */
    confirm_primary_text,
    _("This operation may take a while."),
    _("_Wipe available diskspace"),
    gtk_image_new_from_stock (GTK_STOCK_CLEAR, GTK_ICON_SIZE_BUTTON),
    /* progress dialog */
    _("Wiping available diskspace..."),
    /* operation launcher */
    nautilus_wipe_fill_operation,
    /* failed dialog */
    _("Wipe failed"),
    /* success dialog */
    _("Wipe successful"),
    success_secondary_text
  );
  g_free (confirm_primary_text);
  g_free (success_secondary_text);
}


static void
wipe_menu_item_activate_handler (GObject *item,
                                 gpointer data)
{
  nautilus_wipe_run_delete_operation (g_object_get_data (item, ITEM_DATA_WINDOW_KEY),
                                      g_object_get_data (item, ITEM_DATA_PATHS_KEY));
}

static NautilusMenuItem *
create_wipe_menu_item (NautilusMenuProvider *provider,
                       const gchar          *item_name,
                       GtkWidget            *window,
                       GList                *paths)
{
  NautilusMenuItem *item;
  
  item = nautilus_menu_item_new (item_name,
                                 _("Wipe"),
                                 _("Delete each selected item and overwrite its data"),
                                 GTK_STOCK_DELETE);
  g_object_set_data (G_OBJECT (item), ITEM_DATA_WINDOW_KEY, window);
  g_object_set_data_full (G_OBJECT (item), ITEM_DATA_PATHS_KEY,
                          nautilus_wipe_path_list_copy (paths),
                          (GDestroyNotify) nautilus_wipe_path_list_free);
  g_signal_connect (item, "activate",
                    G_CALLBACK (wipe_menu_item_activate_handler), NULL);
  
  return item;
}

static void
fill_menu_item_activate_handler (GObject *item,
                                 gpointer data)
{
  nautilus_wipe_run_fill_operation (g_object_get_data (item, ITEM_DATA_WINDOW_KEY),
                                    g_object_get_data (item, ITEM_DATA_PATHS_KEY),
                                    g_object_get_data (item, ITEM_DATA_MOUNTPOINTS_KEY));
}

static NautilusMenuItem *
create_fill_menu_item (NautilusMenuProvider *provider,
                       const gchar          *item_name,
                       GtkWidget            *window,
                       GList                *files)
{
  NautilusMenuItem *item        = NULL;
  GList            *mountpoints = NULL;
  GList            *folders     = NULL;
  GError           *err         = NULL;
  
  if (! nautilus_wipe_fill_operation_filter_files (files, &folders,
                                                   &mountpoints, &err)) {
    g_warning (_("File filtering failed: %s"), err->message);
    g_error_free (err);
  } else {
    item = nautilus_menu_item_new (item_name,
                                   _("Wipe available diskspace"),
                                   _("Overwrite available diskspace in this device(s)"),
                                   GTK_STOCK_CLEAR);
    g_object_set_data (G_OBJECT (item), ITEM_DATA_WINDOW_KEY, window);
    g_object_set_data_full (G_OBJECT (item), ITEM_DATA_PATHS_KEY,
                            folders,
                            (GDestroyNotify) nautilus_wipe_path_list_free);
    g_object_set_data_full (G_OBJECT (item), ITEM_DATA_MOUNTPOINTS_KEY,
                            mountpoints,
                            (GDestroyNotify) nautilus_wipe_path_list_free);
    g_signal_connect (item, "activate",
                      G_CALLBACK (fill_menu_item_activate_handler), NULL);
  }
  
  return item;
}

/* adds @item to the #GList @items if not %NULL */
#define ADD_ITEM(items, item)                         \
  G_STMT_START {                                      \
    NautilusMenuItem *ADD_ITEM__item = (item);        \
                                                      \
    if (ADD_ITEM__item != NULL) {                     \
      items = g_list_append (items, ADD_ITEM__item);  \
    }                                                 \
  } G_STMT_END

/* populates Nautilus' file menu */
static GList *
nautilus_wipe_real_get_file_items (NautilusMenuProvider *provider,
                                   GtkWidget            *window,
                                   GList                *files)
{
  GList *items = NULL;
  GList *paths;
  
  paths = nautilus_wipe_path_list_new_from_nfi_list (files);
  if (paths) {
    ADD_ITEM (items, create_wipe_menu_item (provider,
                                            "nautilus-wipe::files-items::wipe",
                                            window, paths));
    ADD_ITEM (items, create_fill_menu_item (provider,
                                            "nautilus-wipe::files-items::fill",
                                            window, paths));
  }
  nautilus_wipe_path_list_free (paths);
  
  return items;
}

/* populates Nautilus' background menu */
static GList *
nautilus_wipe_real_get_background_items (NautilusMenuProvider *provider,
                                         GtkWidget            *window,
                                         NautilusFileInfo     *current_folder)
{
  GList *items = NULL;
  GList *paths = NULL;
  
  paths = g_list_append (paths, nautilus_wipe_path_from_nfi (current_folder));
  if (paths && paths->data) {
    ADD_ITEM (items, create_fill_menu_item (provider,
                                            "nautilus-wipe::background-items::fill",
                                            window, paths));
  }
  nautilus_wipe_path_list_free (paths);
  
  return items;
}

#undef ADD_ITEM
