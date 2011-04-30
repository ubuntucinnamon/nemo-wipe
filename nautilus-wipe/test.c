
#include <gtk/gtk.h>
#include "progress-dialog.h"

#define STEP 0.001

static gdouble fraction = 0.0;

static gboolean
progress (gpointer data)
{
  NautilusWipeProgressDialog *dialog = NAUTILUS_WIPE_PROGRESS_DIALOG (data);
  
  /*fraction = fraction >= (1.0 - STEP) ? 0.0 : fraction + STEP;*/
  fraction += STEP;
  
  if (fraction >= 1.0) {
    nautilus_wipe_progress_dialog_finish (dialog, TRUE);
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
    
    nautilus_wipe_progress_dialog_set_fraction (dialog, fraction);
    nautilus_wipe_progress_dialog_set_progress_text (dialog, "%s", texts[(gint)(fraction * 10)]);
  }
  
  return ! nautilus_wipe_progress_dialog_is_canceled (dialog) &&
         ! nautilus_wipe_progress_dialog_is_finished (dialog);
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
      nautilus_wipe_progress_dialog_set_has_close_button (NAUTILUS_WIPE_PROGRESS_DIALOG (dialog),
                                                          ! nautilus_wipe_progress_dialog_get_has_close_button (NAUTILUS_WIPE_PROGRESS_DIALOG (dialog)));
      break;
    
    case NAUTILUS_WIPE_PROGRESS_DIALOG_RESPONSE_COMPLETE:
      nautilus_wipe_progress_dialog_set_progress_text (NAUTILUS_WIPE_PROGRESS_DIALOG (dialog), "Done!");
      break;
  }
}

int
main (int     argc,
      char  **argv)
{
  GtkWidget *dialog;
  
  gtk_init (&argc, &argv);
  
  dialog = nautilus_wipe_progress_dialog_new (NULL, 0, "Progress...");
  /*nautilus_wipe_progress_dialog_set_has_cancel_button (NAUTILUS_WIPE_PROGRESS_DIALOG (dialog), TRUE);*/
  /*nautilus_wipe_progress_dialog_set_has_close_button (NAUTILUS_WIPE_PROGRESS_DIALOG (dialog), TRUE);*/
  g_signal_connect (dialog, "response", G_CALLBACK (response_handler), NULL);
  gtk_widget_show (GTK_WIDGET (dialog));
  
  g_timeout_add (20, progress, dialog);
  gtk_main ();
  
  return 0;
}

