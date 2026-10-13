/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * What fprintd would receive during an enroll. The headless device runs no
 * libfprint action, so the reporting calls are replaced and recorded here.
 */
#include "sim-internal.h"

GArray  *sim_progress;
GError  *sim_completion;
gboolean sim_completed;

void
fpi_device_enroll_progress (FpDevice *device, gint completed_stages, FpPrint *print, GError *error)
{
  SimProgress p = { completed_stages, error ? error->code : -1 };

  /* The same contract libfprint enforces: only retry errors, never a print together with one. */
  g_assert_true (error == NULL || error->domain == FP_DEVICE_RETRY);
  g_assert_true (error == NULL || print == NULL);
  g_array_append_val (sim_progress, p);
  g_clear_error (&error);
  if (print)
    g_object_unref (g_object_ref_sink (print));
}

void
fpi_device_enroll_complete (FpDevice *device, FpPrint *print, GError *error)
{
  g_assert_false (sim_completed);
  sim_completed = TRUE;
  sim_completion = error;
  g_clear_object (&print);
}

static gboolean
wake_up (gpointer data)
{
  return G_SOURCE_CONTINUE;  /* only makes the blocking iteration check the time limit */
}

const GError *
sim_run_enroll (FpiDeviceElan0c4d *device)
{
  gint64 limit = g_get_monotonic_time () + 20 * G_USEC_PER_SEC;
  guint wake = g_timeout_add (100, wake_up, NULL);

  elan_action_enroll (FP_DEVICE (device));
  while (!sim_completed && g_get_monotonic_time () < limit)
    g_main_context_iteration (NULL, TRUE);
  g_source_remove (wake);
  g_assert_true (sim_completed);
  return sim_completion;
}
