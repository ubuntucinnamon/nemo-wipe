
#include <gtk/gtk.h>
#include "progress-dialog.h"

#define STEP 0.001

static gdouble fraction = 0.0;

static gboolean
progress (gpointer data)
{
  NautilusSrmProgressDialog *dialog = NAUTILUS_SRM_PROGRESS_DIALOG (data);
  
  /*fraction = fraction >= (1.0 - STEP) ? 0.0 : fraction + STEP;*/
  fraction += STEP;
  
  if (fraction >= 1.0) {
    nautilus_srm_progress_dialog_finish (dialog, TRUE);
  } else {
    static const gchar *texts[10] = {
      "Checking input data integrity",
      "Preparing environment",
      "Setting up operation",
      "Listing files",
      "Deleting files",
      "Deleting directories",
      "Cleaning operation",
      "Cleaning environment",
      "Flushing data",
      "Checking output integrity"
    };
    
    nautilus_srm_progress_dialog_set_fraction (dialog, fraction);
    nautilus_srm_progress_dialog_set_progress_text (dialog, "%s", texts[(gint)(fraction * 10)]);
  }
  
  return ! nautilus_srm_progress_dialog_is_canceled (dialog) &&
         ! nautilus_srm_progress_dialog_is_finished (dialog);
}

static void
response_handler (GtkDialog *dialog,
                  gint       response_id,
                  gpointer   data)
{
  g_debug ("I got response %d", response_id);
  
  switch (response_id) {
    case GTK_RESPONSE_CANCEL:
    case GTK_RESPONSE_CLOSE:
      gtk_main_quit ();
      break;
    
    case GTK_RESPONSE_DELETE_EVENT:
      nautilus_srm_progress_dialog_set_has_close_button (NAUTILUS_SRM_PROGRESS_DIALOG (dialog),
                                                         ! nautilus_srm_progress_dialog_get_has_close_button (NAUTILUS_SRM_PROGRESS_DIALOG (dialog)));
      break;
    
    case NAUTILUS_SRM_PROGRESS_DIALOG_RESPONSE_COMPLETE:
      nautilus_srm_progress_dialog_set_progress_text (NAUTILUS_SRM_PROGRESS_DIALOG (dialog), "Done!");
      break;
  }
}

int
main (int     argc,
      char  **argv)
{
  GtkWidget *dialog;
  
  gtk_init (&argc, &argv);
  
  dialog = nautilus_srm_progress_dialog_new (NULL, 0, "Progress...");
  /*nautilus_srm_progress_dialog_set_has_cancel_button (NAUTILUS_SRM_PROGRESS_DIALOG (dialog), TRUE);*/
  /*nautilus_srm_progress_dialog_set_has_close_button (NAUTILUS_SRM_PROGRESS_DIALOG (dialog), TRUE);*/
  g_signal_connect (dialog, "response", G_CALLBACK (response_handler), NULL);
  gtk_widget_show (GTK_WIDGET (dialog));
  
  g_timeout_add (20, progress, dialog);
  gtk_main ();
  
  return 0;
}

