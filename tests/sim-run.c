/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* Starting a simulated device and running state machines to completion. */
#include "sim-internal.h"

guint sim_warning_count;
static GMainLoop *loop;
static GError *run_error;
static gboolean finished;

static void
count_warning (const gchar *domain, GLogLevelFlags level, const gchar *message, gpointer data)
{
  sim_warning_count++;
  g_test_message ("warning: %s", message);
}

FpiDeviceElan0c4d *
sim_start (SimSensor *sensor)
{
  static gboolean handler_installed;

  sim = sensor;
  sim->wire = g_string_new (NULL);
  sim_progress = g_array_new (FALSE, FALSE, sizeof (SimProgress));
  sim_completed = FALSE;
  sim_warning_count = 0;
  g_clear_object (&sim_cancellable);
  sim_cancellable = g_cancellable_new ();
  if (!handler_installed)
    {
      /* The driver warns on purpose when it recovers; criticals stay fatal. */
      g_log_set_always_fatal (G_LOG_FATAL_MASK | G_LOG_LEVEL_CRITICAL);
      g_log_set_handler (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING, count_warning, NULL);
      handler_installed = TRUE;
    }
  return g_object_new (FPI_TYPE_DEVICE_ELAN0C4D, NULL);
}

static void
run_done (FpiSsm *ssm, FpDevice *dev, GError *error)
{
  run_error = error;
  finished = TRUE;
  g_main_loop_quit (loop);
}

static gboolean
run_guard (gpointer data)
{
  g_error ("sim: the state machine did not finish in 20 s");
  return G_SOURCE_REMOVE;
}

GError *
sim_run (FpiSsm *ssm)
{
  guint guard = g_timeout_add_seconds (20, run_guard, NULL);

  loop = g_main_loop_new (NULL, FALSE);
  finished = FALSE;
  run_error = NULL;
  fpi_ssm_start (ssm, run_done);
  if (!finished)  /* a machine can end inside fpi_ssm_start (refused command) */
    g_main_loop_run (loop);
  g_source_remove (guard);
  g_main_loop_unref (loop);
  return run_error;
}

static gboolean
cancel_now (gpointer data)
{
  g_string_append (sim->wire, " [cancel]");  /* what the driver writes after it is visible */
  g_cancellable_cancel (sim_cancellable);
  return G_SOURCE_REMOVE;
}

void
sim_cancel_in (guint ms)
{
  g_timeout_add (ms, cancel_now, NULL);
}

guint
sim_warnings (void)
{
  return sim_warning_count;
}

void
sim_end (FpiDeviceElan0c4d *device)
{
  g_object_unref (device);
  g_string_free (sim->wire, TRUE);
  g_clear_pointer (&sim_progress, g_array_unref);
  g_clear_error (&sim_completion);
  g_clear_object (&sim_enrolled);
}
