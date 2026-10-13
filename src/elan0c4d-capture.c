/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Finger capture (elan_capture.py capture_finger). 0xAF/0x55 only say "look
 * now": the frame decides (classifier). Invalid frames are recalibrated and
 * reinitialized inside FRAME; false triggers re-arm the wait until the
 * deadline. A finger already resting is captured at once, so the flow never
 * depends on the user reading a prompt in time.
 */
#include "elan0c4d-ops.h"

enum { CAP_PREPARE, CAP_START, CAP_FRAME, CAP_DECIDE, CAP_ARM, CAP_AGAIN, CAP_STATES };

static void
capture_handler (FpiSsm *ssm, FpDevice *dev)
{
  FpiDeviceElan0c4d *self = FPI_DEVICE_ELAN0C4D (dev);

  switch (fpi_ssm_get_cur_state (ssm))
    {
    case CAP_PREPARE:
      fpi_ssm_start_subsm (ssm, elan_retry_ssm_new (dev, "prepare", elan_prepare_ssm_new, NULL, TRUE));
      break;

    case CAP_START:
      self->deadline_us = g_get_monotonic_time () + (gint64) self->capture_timeout_ms * 1000;
      fpi_ssm_next_state (ssm);
      break;

    case CAP_FRAME:
      fpi_ssm_start_subsm (ssm, elan_frame_ssm_new (dev, NULL));
      break;

    case CAP_DECIDE:
      if (self->assessment.kind == ELAN_FRAME_FINGER)
        fpi_ssm_mark_completed (ssm);
      else if (elan_capture_left_ms (self) <= 0)
        fpi_ssm_mark_failed (ssm, elan_no_finger_error (self));
      else
        fpi_ssm_next_state (ssm);
      break;

    case CAP_ARM:
      fpi_ssm_start_subsm (ssm, elan_arm_ssm_new (dev, NULL));
      break;

    default:
      g_debug ("wait code 0x%02x, checking the frame", self->wait_code);
      fpi_ssm_jump_to_state (ssm, CAP_FRAME);
    }
}

/* Frame, background and classification are left in the device (frame, background, assessment). */
FpiSsm *
elan_capture_ssm_new (FpDevice *dev)
{
  return fpi_ssm_new_full (dev, capture_handler, CAP_STATES, CAP_STATES, "elan-capture");
}
