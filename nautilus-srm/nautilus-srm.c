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

#include "nautilus-srm.h"

#include <libnautilus-extension/nautilus-menu-provider.h>
#include <libnautilus-extension/nautilus-file-info.h>

#include <glib.h>
#define GETTEXT_PACKAGE "nautilus-srm"
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>

#include <gio/gio.h>

#include <gsecuredelete/gsecuredelete.h>

#include "operation-manager.h"
#include "delete-operation.h"
#include "fill-operation.h"

/* if GLib doesn't provide g_dngettext(), wrap it from dngettext() */
#if (! GLIB_CHECK_VERSION (2, 18, 0) && ! defined (g_dngettext))
# include <libintl.h>
# define g_dngettext dngettext
#endif


static GType provider_types[1];
static GType nautilus_srm_type = 0;

/* private prototypes */
static void   nautilus_srm_register_type     (GTypeModule *module);
static GList *nautilus_srm_get_file_items    (NautilusMenuProvider *provider,
                                              GtkWidget            *window,
                                              GList                *files);
static GList *nautilus_srm_get_background_items (NautilusMenuProvider *provider,
                                              GtkWidget            *window,
                                              NautilusFileInfo     *current_folder);

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
  iface->get_background_items = nautilus_srm_get_background_items;
}

static void 
nautilus_srm_instance_init (NautilusSrm *srm)
{
  g_message ("Object [%p] initialized", srm);
}

static void 
nautilus_srm_instance_finalize (GObject *object)
{
  g_message ("Object [%p] finalized", object);
}

static void
nautilus_srm_class_init (NautilusSrmClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  
  object_class->finalize = nautilus_srm_instance_finalize;
  
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
static void
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

static void       run_fill_operation    (GtkWindow *parent,
                                         GList     *files);
static void       run_delete_operation  (GtkWindow *parent,
                                         GList     *files);



/* Data needed to be able to start an operation.
 * We don't use private filed of the extension's object because there is only
 * one extension object for more than one window. Actually, it would attach all
 * operations to the same window rather than the one that launched it. */
struct ItemData
{
  GtkWindow *window;  /* parent window */
  GList     *files;   /* list of selected NautilusFileInfos */
};

/* Frees an #ItemData */
static void
item_data_free (struct ItemData *idata)
{
  g_object_unref (idata->window);
  nautilus_file_info_list_free (idata->files);
  g_slice_free1 (sizeof *idata, idata);
}

/*
 * Attaches a new #ItemData to a #NautilusMenuItem.
 * It is freed automatically when @item is destroyed.
 */
static void
add_item_data (NautilusMenuItem *item,
               GtkWindow        *window,
               GList            *files)
{
  struct ItemData *idata;
  
  idata = g_slice_alloc (sizeof *idata);
  idata->window = g_object_ref (window);
  idata->files = nautilus_file_info_list_copy (files);
  g_object_set_data_full (G_OBJECT (item), "NautilusSrm::item-data",
                          idata, (GDestroyNotify)item_data_free);
}

/* Gets the #ItemData attached to @item */
static struct ItemData *
get_item_data (NautilusMenuItem *item)
{
  return g_object_get_data (G_OBJECT (item), "NautilusSrm::item-data");
}


/*= Menu items =*/

/* srm item */
static void
menu_item_delete_activate_handler (NautilusMenuItem     *item,
                                   NautilusMenuProvider *provider)
{
  struct ItemData *idata = get_item_data (item);
  
  run_delete_operation (idata->window, idata->files);
}

static NautilusMenuItem *
nautilus_srm_menu_item_srm (NautilusMenuProvider *provider,
                            GtkWidget            *window,
                            GList                *files)
{
  NautilusMenuItem *item;
  
  item = nautilus_menu_item_new ("NautilusSrm::srm_item",
                                 _("Delete and override content"),
                                 g_dngettext (NULL, "Delete the selected file and override its data",
                                                    "Delete the selected files and override their data",
                                                    g_list_length (files)),
                                 GTK_STOCK_DELETE);
  add_item_data (item, GTK_WINDOW (window), files);
  g_signal_connect (item, "activate",
                    G_CALLBACK (menu_item_delete_activate_handler), provider);
  
  return item;
}

/* sfill item */
static void
menu_item_fill_activate_handler (NautilusMenuItem     *item,
                                 NautilusMenuProvider *provider)
{
  struct ItemData *idata = get_item_data (item);
  
  run_fill_operation (idata->window, idata->files);
}

static NautilusMenuItem *
nautilus_srm_menu_item_sfill (NautilusMenuProvider *provider,
                              GtkWidget            *window,
                              GList                *folders)
{
  NautilusMenuItem *item;

  item = nautilus_menu_item_new ("NautilusSrm::sfill_item",
                                 _("Override free space here"),
                                 g_dngettext (NULL, "Override free space in the device containing this file",
                                                    "Override free space in the device(s) containing this files",
                                                    g_list_length (folders)),
                                 NULL);
  add_item_data (item, GTK_WINDOW (window), folders);
  g_signal_connect (item, "activate",
                    G_CALLBACK (menu_item_fill_activate_handler), provider);
  
  return item;
}

/* populates Nautilus' file menu */
static GList *
nautilus_srm_get_file_items (NautilusMenuProvider *provider,
                             GtkWidget            *window,
                             GList                *files)
{
  GList *items = NULL;
  items = g_list_append (items, nautilus_srm_menu_item_srm (provider,
                                                            window, files));
  items = g_list_append (items, nautilus_srm_menu_item_sfill (provider,
                                                              window, files));
  
  return items;
}

/* populates Nautilus' background menu */
static GList *
nautilus_srm_get_background_items (NautilusMenuProvider *provider,
                                   GtkWidget            *window,
                                   NautilusFileInfo     *current_folder)
{
  GList *items = NULL;
  GList *files = g_list_append (NULL, current_folder);
  
  items = g_list_append (items, nautilus_srm_menu_item_sfill (provider,
                                                              window, files));
  g_list_free (files);
  
  return items;
}



/* Runs the srm operation */
static void
run_delete_operation (GtkWindow *parent,
                      GList     *files)
{
  gchar  *confirm_primary_text = NULL;
  guint   n_items;
  
  n_items = g_list_length (files);
  /* FIXME: can't truly use g_dngettext since the args are not the same */
  if (n_items > 1) {
    confirm_primary_text = g_strdup_printf (_("Are you sure you want to delete and wipe "
                                              "the %u selected items?"), n_items);
  } else if (n_items > 0) {
    gchar *name;
    
    name = nautilus_file_info_get_name (files->data);
    confirm_primary_text = g_strdup_printf (_("Are you sure you want to delete and wipe "
                                              "\"%s\"?"),
                                            name);
    g_free (name);
  }
  nautilus_srm_operation_manager_run (
    parent, files,
    /* confirm dialog */
    confirm_primary_text,
    _("If you delete an item, it will not be recoverable."),
    GTK_STOCK_DELETE,
    /* progress dialog */
    _("Deleting files..."),
    /* operation launcher */
    nautilus_srm_delete_operation,
    /* failed dialog */
    _("Deletion failed"),
    /* success dialog */
    _("Deletion succeeded"),
    _("Files have been successfully deleted and wiped")
  );
  g_free (confirm_primary_text);
}

/* Runs the sfill operation */
static void
run_fill_operation (GtkWindow *parent,
                    GList     *files)
{
  gchar  *confirm_primary_text = NULL;
  guint   n_items;
  
  n_items = g_list_length (files);
  /* FIXME: can't truly use g_dngettext since the args are not the same */
  if (n_items > 1) {
    confirm_primary_text = g_strdup_printf (_("Are you sure you want to wipe the free space "
                                              "on the device(s) of the %u selected items?"),
                                            n_items);
  } else if (n_items > 0) {
    gchar *name;
    
    name = nautilus_file_info_get_name (files->data);
    confirm_primary_text = g_strdup_printf (_("Are you sure you want to wipe the free space "
                                              "on the device of \"%s\"?"),
                                            name);
    g_free (name);
  }
  nautilus_srm_operation_manager_run (
    parent, files,
    /* confirm dialog */
    confirm_primary_text,
    _("This operation may take a while."),
    _("Overwrite free space"),
    /* progress dialog */
    _("Wiping free space..."),
    /* operation launcher */
    nautilus_srm_fill_operation,
    /* failed dialog */
    _("Wipe failed"),
    /* success dialog */
    _("Wipe succeeded"),
    _("Free space on the device(s) have been successfully wiped")
  );
  g_free (confirm_primary_text);
}
