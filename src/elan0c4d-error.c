/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include <gio/gio.h>
#include <gusb.h>
#include "fpi-device.h"
#include "elan0c4d-error.h"

G_DEFINE_QUARK (elan0c4d-error-quark, elan_error)

gboolean
elan_error_is_timeout (const GError *error)
{
  return g_error_matches (error, ELAN_ERROR, ELAN_ERROR_TIMEOUT) ||
         g_error_matches (error, G_USB_DEVICE_ERROR, G_USB_DEVICE_ERROR_TIMED_OUT);
}

/*
 * A vanished device cannot be reopened from inside libfprint (the FpDevice is
 * removed on hotplug), so NO_DEVICE, BUSY and cancellations are never retried.
 */
ElanRemedy
elan_error_remedy (const GError *error)
{
  if (g_error_matches (error, ELAN_ERROR, ELAN_ERROR_KEY_LOST))
    return ELAN_REMEDY_REKEY;
  if (error->domain == ELAN_ERROR)
    return error->code == ELAN_ERROR_PROTOCOL ||
           error->code == ELAN_ERROR_TIMEOUT ||
           error->code == ELAN_ERROR_CALIBRATION ? ELAN_REMEDY_REINIT : ELAN_REMEDY_NONE;
  if (g_error_matches (error, G_USB_DEVICE_ERROR, G_USB_DEVICE_ERROR_TIMED_OUT))
    return ELAN_REMEDY_REINIT;
  if (g_error_matches (error, G_USB_DEVICE_ERROR, G_USB_DEVICE_ERROR_IO) ||
      g_error_matches (error, G_USB_DEVICE_ERROR, G_USB_DEVICE_ERROR_FAILED))
    return ELAN_REMEDY_REOPEN;
  return ELAN_REMEDY_NONE;
}

static FpDeviceError
device_code (const GError *error)
{
  if (g_error_matches (error, G_USB_DEVICE_ERROR, G_USB_DEVICE_ERROR_NO_DEVICE))
    return FP_DEVICE_ERROR_REMOVED;
  if (g_error_matches (error, G_USB_DEVICE_ERROR, G_USB_DEVICE_ERROR_BUSY))
    return FP_DEVICE_ERROR_BUSY;
  if (error->domain == ELAN_ERROR && error->code == ELAN_ERROR_TIMEOUT)
    return FP_DEVICE_ERROR_GENERAL;
  return FP_DEVICE_ERROR_PROTO;
}

GError *
elan_error_to_device (GError *error)
{
  GError *out;

  if (error == NULL)
    return NULL;
  if (error->domain == FP_DEVICE_ERROR || error->domain == FP_DEVICE_RETRY ||
      g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
    return error;
  out = fpi_device_error_new_msg (device_code (error), "%s", error->message);
  g_error_free (error);
  return out;
}
