/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * read_frame (elan_capture.py): 00 09 and classify. While the front end
 * returns zeros or saturation (INVALID) recalibrate first and, if it
 * persists, reinitialize and recalibrate; at most two remedies, then the
 * last frame is handed to the caller whatever it is.
 */
#include "elan0c4d-ops.h"

enum { FRAME_GRAB, FRAME_CLASSIFY, FRAME_CHECK, FRAME_REMEDY, FRAME_AGAIN, FRAME_STATES };

static void
frame_handler (FpiSsm *ssm, FpDevice *dev)
{
  FpiDeviceElan0c4d *self = FPI_DEVICE_ELAN0C4D (dev);
  guint *remedies = fpi_ssm_get_data (ssm);
  const ElanAssessment *a = &self->assessment;

  switch (fpi_ssm_get_cur_state (ssm))
    {
    case FRAME_GRAB:
      fpi_ssm_start_subsm (ssm, elan_retry_ssm_new (dev, "image", elan_image_ssm_new, NULL, TRUE));
      break;

    case FRAME_CLASSIFY:
      self->assessment = elan_quality_assess (self->frame, ELAN_WIDTH, ELAN_HEIGHT);
      g_info ("frame mean=%.0f kind=%s coverage=%.2f ridge=%.0f", a->mean,
              elan_frame_kind_name (a->kind), a->coverage, a->ridge);
      fpi_ssm_next_state (ssm);
      break;

    case FRAME_CHECK:  /* recovering is routine (seen every few touches); failing to is not */
      if (a->kind == ELAN_FRAME_INVALID && *remedies >= 2)
        g_warning ("frame still invalid (mean %.0f) after recalibrating and reinitializing", a->mean);
      if (a->kind != ELAN_FRAME_INVALID || *remedies >= 2)
        fpi_ssm_mark_completed (ssm);
      else
        fpi_ssm_next_state (ssm);
      break;

    case FRAME_REMEDY:
      g_info ("invalid frame (mean %.0f): %s", a->mean, *remedies == 0 ? "recalibrating" : "reinitializing");
      fpi_ssm_start_subsm (ssm, *remedies == 0 ?
                           elan_retry_ssm_new (dev, "recalibrate", elan_calibrate_ssm_new, NULL, TRUE) :
                           elan_retry_ssm_new (dev, "reinitialize", elan_reinit_ssm_new, NULL, TRUE));
      (*remedies)++;
      break;

    default:
      fpi_ssm_jump_to_state (ssm, FRAME_GRAB);
    }
}

FpiSsm *
elan_frame_ssm_new (FpDevice *dev, gconstpointer arg)
{
  FpiSsm *ssm = fpi_ssm_new_full (dev, frame_handler, FRAME_STATES, FRAME_STATES, "elan-frame");

  fpi_ssm_set_data (ssm, g_new0 (guint, 1), g_free);
  return ssm;
}
