/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * wait_lift (elan_capture.py): read frames until the classifier no longer
 * sees a finger; between enroll touches the image is more reliable than
 * 40 3e. With capture_timeout_ms at 0 there is no deadline of its own: it
 * waits until fprintd cancels, like the capture. Cancellation is checked
 * before each 00 09, so no image is ever left unread.
 */
#include "elan0c4d-enroll.h"

enum { LIFT_FRAME, LIFT_CHECK, LIFT_STATES };

static void
lift_check (FpiSsm *ssm, FpiDeviceElan0c4d *self, gint64 deadline_us)
{
  if (self->assessment.kind != ELAN_FRAME_FINGER)
    {
      g_info ("finger lifted (%s frame)", elan_frame_kind_name (self->assessment.kind));
      fpi_ssm_mark_completed (ssm);
    }
  else if (self->capture_timeout_ms > 0 && g_get_monotonic_time () >= deadline_us)
    fpi_ssm_mark_failed (ssm, g_error_new (ELAN_ERROR, ELAN_ERROR_TIMEOUT, "finger not lifted in %d ms",
                                           self->capture_timeout_ms));
  else
    fpi_ssm_jump_to_state_delayed (ssm, LIFT_FRAME, ELAN_LIFT_POLL_MS);
}

static void
lift_handler (FpiSsm *ssm, FpDevice *dev)
{
  const gint64 *deadline_us = fpi_ssm_get_data (ssm);

  if (g_cancellable_is_cancelled (fpi_device_get_cancellable (dev)))
    fpi_ssm_mark_failed (ssm, g_error_new_literal (G_IO_ERROR, G_IO_ERROR_CANCELLED,
                                                   "cancelled while waiting for the finger to lift"));
  else if (fpi_ssm_get_cur_state (ssm) == LIFT_FRAME)
    fpi_ssm_start_subsm (ssm, elan_frame_ssm_new (dev, NULL));
  else
    lift_check (ssm, FPI_DEVICE_ELAN0C4D (dev), *deadline_us);
}

/* The classification of the last frame read is left in self->assessment. */
FpiSsm *
elan_lift_ssm_new (FpDevice *dev, gconstpointer arg)
{
  FpiSsm *ssm = fpi_ssm_new_full (dev, lift_handler, LIFT_STATES, LIFT_STATES, "elan-lift");
  gint64 *deadline_us = g_new (gint64, 1);

  *deadline_us = g_get_monotonic_time () + (gint64) FPI_DEVICE_ELAN0C4D (dev)->capture_timeout_ms * 1000;
  fpi_ssm_set_data (ssm, deadline_us, g_free);
  return ssm;
}
