/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Open: claim interface 0, initialize and calibrate (with retries). Close: a
 * best-effort 00 0b so the MCU is not left waiting, then release the interface.
 * Failures on the way are logged and never leave the interface claimed.
 */
#include <gusb.h>
#include "elan0c4d-ops.h"

static void
release_interface (FpDevice *dev, GError **error)
{
  FpiDeviceElan0c4d *self = FPI_DEVICE_ELAN0C4D (dev);

  if (!self->claimed)
    return;
  self->claimed = FALSE;
  if (!g_usb_device_release_interface (fpi_device_get_usb_device (dev), 0, 0, error))
    g_warning ("release interface: %s", (*error)->message);
}

static void
open_done (FpiSsm *ssm, FpDevice *dev, GError *error)
{
  g_autoptr(GError) ignored = NULL;

  if (error)
    {
      elan_log_error ("open", error);
      release_interface (dev, &ignored);
    }
  else
    g_info ("sensor open, initialized and calibrated");
  fpi_device_open_complete (dev, elan_error_to_device (error));
}

void
elan_open (FpDevice *dev)
{
  GError *error = NULL;

  if (!g_usb_device_claim_interface (fpi_device_get_usb_device (dev), 0, 0, &error))
    {
      elan_log_error ("claim interface", error);
      fpi_device_open_complete (dev, elan_error_to_device (error));
      return;
    }
  FPI_DEVICE_ELAN0C4D (dev)->claimed = TRUE;
  g_info ("opening ELAN 04f3:0c4d rev 0x%04x",
          g_usb_device_get_release (fpi_device_get_usb_device (dev)));
  fpi_ssm_start (elan_retry_ssm_new (dev, "open", elan_reinit_ssm_new, NULL, TRUE), open_done);
}

static void
close_done (FpiSsm *ssm, FpDevice *dev, GError *error)
{
  GError *release_error = NULL;

  if (error)
    {
      g_info ("cancel on close ignored: %s", error->message);
      g_error_free (error);
    }
  release_interface (dev, &release_error);
  fpi_device_close_complete (dev, elan_error_to_device (release_error));
}

void
elan_close (FpDevice *dev)
{
  fpi_ssm_start (elan_cmd_ssm_new (dev, &elan_cancel_cmd), close_done);
}
