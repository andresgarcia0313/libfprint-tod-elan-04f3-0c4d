/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Every action captures one usable finger frame (elan0c4d-capture.c) and then
 * asks the comparator interface. With the stub the action ends with
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
enroll_done (FpiSsm *ssm, FpDevice *dev, GError *error)
{
  ElanFrame frame = frame_of (FPI_DEVICE_ELAN0C4D (dev));
  g_autoptr(GBytes) tmpl = NULL;
  FpPrint *print = NULL;

  if (error == NULL && elan_match_enroll (&frame, &tmpl, &error))
    {
      fpi_device_get_enroll_data (dev, &print);
      elan_print_store (print, tmpl);
      fpi_device_enroll_complete (dev, g_object_ref (print), NULL);
      return;
    }
  elan_log_error ("enroll", error);
  fpi_device_enroll_complete (dev, NULL, elan_error_to_device (error));
}

static void
verify_done (FpiSsm *ssm, FpDevice *dev, GError *error)
{
  ElanFrame frame = frame_of (FPI_DEVICE_ELAN0C4D (dev));
  g_autoptr(GBytes) tmpl = NULL;
  FpPrint *print = NULL;
  gboolean matched = FALSE;

  fpi_device_get_verify_data (dev, &print);
  tmpl = elan_print_load (print);
  if (error == NULL && tmpl == NULL)
    error = fpi_device_error_new_msg (FP_DEVICE_ERROR_DATA_INVALID, "the print holds no ELAN template");
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
      g_autoptr(GBytes) tmpl = elan_print_load (g_ptr_array_index (prints, i));
      gboolean matched = FALSE;

      if (tmpl && elan_match_verify (&frame, tmpl, &matched, &error) && matched)
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
elan_action_enroll (FpDevice *dev)
{
  fpi_ssm_start (elan_capture_ssm_new (dev), enroll_done);
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
