/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* Enroll scripts for the simulated sensor: touches in, progress out. */
#include "sim.h"
#include "elan0c4d-enroll.h"

#define MAX_TOUCHES (ELAN_ENROLL_STAGES + 4)  /* a full enroll plus its retries */

static SimImage images[MAX_TOUCHES * 3];

/* Each touch: the background of the prepare, the touch itself, an empty frame for the lift. */
FpiDeviceElan0c4d *
sim_start_touches (SimSensor *s, const SimImage *touches, guint n)
{
  static const guint8 ff[] = { 0xFF };

  g_assert_cmpuint (n, <=, MAX_TOUCHES);
  for (guint i = 0; i < n; i++)
    {
      images[3 * i] = SIM_EMPTY;
      images[3 * i + 1] = touches[i];
      images[3 * i + 2] = SIM_EMPTY;
    }
  *s = (SimSensor) { .images = images, .n_images = 3 * n, .press = ff, .n_press = 1 };
  return sim_start (s);
}

/* `retry_at` gets `retry` (FpDeviceRetry) and repeats the previous count; every other touch counts. */
void
sim_assert_progress (guint n, guint retry_at, gint retry)
{
  g_assert_cmpuint (sim_progress->len, ==, n);
  for (guint i = 0, stages = 0; i < n; i++)
    {
      SimProgress p = g_array_index (sim_progress, SimProgress, i);

      stages += i == retry_at ? 0 : 1;
      g_assert_cmpint (p.stages, ==, stages);
      g_assert_cmpint (p.retry, ==, i == retry_at ? retry : -1);
    }
}
