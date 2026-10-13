/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Enroll stages, like a phone: capture a touch, report it to fprintd, wait
 * until the finger is lifted, again until ELAN_ENROLL_STAGES are accepted.
 * A touch with too little finger, or the same contact as the previous stage
 * (finger not moved), is reported as a retry and does not count.
 */
#include "elan0c4d-enroll.h"

enum { ENR_START, ENR_CAPTURE, ENR_JUDGE, ENR_LIFT, ENR_AGAIN, ENR_STATES };

/* NULL if the touch in self->frame (8-bit image in `image`) counts as a stage. */
static GError *
stage_retry (FpiDeviceElan0c4d *self, const guint8 *image)
{
  const ElanEnrollment *e = self->enrollment;
  gdouble coverage = self->assessment.coverage;
  gdouble similar = e->n ? elan_quality_similarity (e->normalized[e->n - 1], image, ELAN_PIXELS) : 0.0;

  g_info ("touch for stage %u: coverage %.2f, similarity to the previous %.2f", e->n + 1, coverage, similar);
  if (coverage < ELAN_ENROLL_COVERAGE_MIN)
    return fpi_device_retry_new_msg (FP_DEVICE_RETRY_CENTER_FINGER, "coverage %.2f below %.2f",
                                     coverage, ELAN_ENROLL_COVERAGE_MIN);
  if (similar >= ELAN_ENROLL_SIMILAR_MAX)
    return fpi_device_retry_new_msg (FP_DEVICE_RETRY_REMOVE_FINGER, "same touch as stage %u (%.2f)",
                                     e->n, similar);
  return NULL;
}

static void
judge (FpiSsm *ssm, FpiDeviceElan0c4d *self)
{
  ElanEnrollment *e = self->enrollment;
  GError *retry;

  elan_quality_normalize (self->frame, ELAN_WIDTH, ELAN_HEIGHT, e->normalized[e->n]);
  retry = stage_retry (self, e->normalized[e->n]);
  if (retry)
    g_info ("touch rejected: %s", retry->message);
  else
    {
      memcpy (e->pixels[e->n], self->frame, sizeof e->pixels[0]);
      memcpy (e->background[e->n], self->background, sizeof e->background[0]);
      e->n++;
      g_info ("enroll stage %u of %d accepted", e->n, ELAN_ENROLL_STAGES);
    }
  fpi_device_enroll_progress (FP_DEVICE (self), e->n, NULL, retry);
  if (e->n == ELAN_ENROLL_STAGES)
    fpi_ssm_mark_completed (ssm);
  else
    fpi_ssm_next_state (ssm);
}

static void
start (FpiSsm *ssm, FpiDeviceElan0c4d *self)
{
  g_clear_pointer (&self->enrollment, g_free);
  self->enrollment = g_try_new0 (ElanEnrollment, 1);
  if (self->enrollment)
    fpi_ssm_next_state (ssm);
  else
    fpi_ssm_mark_failed (ssm, fpi_device_error_new_msg (FP_DEVICE_ERROR_GENERAL,
                                                        "no memory for %d enroll stages", ELAN_ENROLL_STAGES));
}

static void
stages_handler (FpiSsm *ssm, FpDevice *dev)
{
  FpiDeviceElan0c4d *self = FPI_DEVICE_ELAN0C4D (dev);

  switch (fpi_ssm_get_cur_state (ssm))
    {
    case ENR_START:
      start (ssm, self);
      break;

    case ENR_CAPTURE:
      fpi_ssm_start_subsm (ssm, elan_capture_ssm_new (dev));
      break;

    case ENR_JUDGE:
      judge (ssm, self);
      break;

    case ENR_LIFT:
      fpi_ssm_start_subsm (ssm, elan_lift_ssm_new (dev, NULL));
      break;

    default:
      fpi_ssm_jump_to_state (ssm, ENR_CAPTURE);
    }
}

FpiSsm *
elan_stages_ssm_new (FpDevice *dev)
{
  return fpi_ssm_new_full (dev, stages_handler, ENR_STATES, ENR_STATES, "elan-stages");
}
