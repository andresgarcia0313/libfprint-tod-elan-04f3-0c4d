/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Verify and identify capture one usable finger frame (elan0c4d-capture.c)
 * and then ask the comparator interface. With the stub they end with
 * FP_DEVICE_ERROR_NOT_SUPPORTED after a full, logged capture.
 */
#include "elan0c4d-actions.h"
#include "elan0c4d-ops.h"

static ElanFrame
frame_of (FpiDeviceElan0c4d *self)
{
  return (ElanFrame) { self->frame, self->background, ELAN_WIDTH, ELAN_HEIGHT };
}

static void
verify_done (FpiSsm *ssm, FpDevice *dev, GError *error)
{
  ElanFrame frame = frame_of (FPI_DEVICE_ELAN0C4D (dev));
  g_autoptr(GBytes) tmpl = NULL;
  FpPrint *print = NULL;
  gboolean matched = FALSE;

  fpi_device_get_verify_data (dev, &print);
  if (error == NULL)
    tmpl = elan_print_load (print, &error);
  if (error == NULL && elan_match_verify (&frame, tmpl, &matched, &error))
    {
      fpi_device_verify_report (dev, matched ? FPI_MATCH_SUCCESS : FPI_MATCH_FAIL, NULL, NULL);
      fpi_device_verify_complete (dev, NULL);
      return;
    }
  elan_log_error ("verify", error);
  fpi_device_verify_complete (dev, elan_error_to_device (error));
}

static void
identify_done (FpiSsm *ssm, FpDevice *dev, GError *error)
{
  ElanFrame frame = frame_of (FPI_DEVICE_ELAN0C4D (dev));
  GPtrArray *prints = NULL;
  FpPrint *found = NULL;

  fpi_device_get_identify_data (dev, &prints);
  for (guint i = 0; error == NULL && found == NULL && i < prints->len; i++)
    {
      g_autoptr(GError) unusable = NULL;
      g_autoptr(GBytes) tmpl = elan_print_load (g_ptr_array_index (prints, i), &unusable);
      gboolean matched = FALSE;

      if (tmpl == NULL)  /* a print of another driver or format is just not a candidate */
        g_debug ("identify: print %u skipped: %s", i, unusable->message);
      else if (elan_match_verify (&frame, tmpl, &matched, &error) && matched)
        found = g_ptr_array_index (prints, i);
    }
  if (error)
    {
      elan_log_error ("identify", error);
      fpi_device_identify_complete (dev, elan_error_to_device (error));
      return;
    }
  fpi_device_identify_report (dev, found, NULL, NULL);
  fpi_device_identify_complete (dev, NULL);
}

void
elan_action_verify (FpDevice *dev)
{
  fpi_ssm_start (elan_capture_ssm_new (dev), verify_done);
}

void
elan_action_identify (FpDevice *dev)
{
  fpi_ssm_start (elan_capture_ssm_new (dev), identify_done);
}
