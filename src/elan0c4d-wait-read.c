/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "elan0c4d-wait.h"

GError *
elan_wait_timeout_error (const Wait *wait)
{
  return g_error_new (ELAN_ERROR, ELAN_ERROR_TIMEOUT, "%s: no decisive code in %u ms",
                      wait->w.name, wait->w.timeout_ms);
}

static void
wait_read_cb (FpiUsbTransfer *transfer, FpDevice *dev, gpointer data, GError *error)
{
  FpiDeviceElan0c4d *self = FPI_DEVICE_ELAN0C4D (dev);
  FpiSsm *ssm = transfer->ssm;
  Wait *wait = fpi_ssm_get_data (ssm);

  if (error && elan_error_is_timeout (error))
    {
      g_clear_error (&error);
      error = elan_wait_timeout_error (wait);
    }
  if (error)
    {
      elan_log_error (wait->w.name, error);
      wait->pending = error;
      fpi_ssm_jump_to_state (ssm, WAIT_ABORT);
    }
  else if (transfer->actual_length <= 0)  /* zero-length packet: no answer yet */
    fpi_ssm_jump_to_state (ssm, WAIT_READ);
  else
    {
      self->resp_len = MIN ((gsize) transfer->actual_length, sizeof self->resp);
      memcpy (self->resp, transfer->buffer, self->resp_len);
      elan_log_hex ("in ", ELAN_EP_INFO, self->resp, self->resp_len);
      fpi_ssm_next_state (ssm);
    }
}

void
elan_wait_read (FpiSsm *ssm, FpDevice *dev, gint64 left_ms)
{
  FpiUsbTransfer *transfer = fpi_usb_transfer_new (dev);

  transfer->ssm = ssm;
  transfer->short_is_error = FALSE;
  fpi_usb_transfer_fill_bulk (transfer, ELAN_EP_INFO, ELAN_RESP_MAX);
  fpi_usb_transfer_submit (transfer, left_ms, fpi_device_get_cancellable (dev), wait_read_cb, NULL);
}
