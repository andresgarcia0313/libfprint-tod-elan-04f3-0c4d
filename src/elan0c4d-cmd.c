/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* One USB exchange: write on EP 0x01 and an optional read. */
#include <gio/gio.h>
#include "elan0c4d-guard.h"
#include "elan0c4d-ops.h"

enum { CMD_WRITE, CMD_READ, CMD_STATES };

const ElanCmd elan_cancel_cmd = ELAN_WRITE ("cancel", elan_cmd_cancel);

static void
cmd_write_cb (FpiUsbTransfer *transfer, FpDevice *dev, gpointer data, GError *error)
{
  if (error)
    elan_log_error (((const ElanCmd *) fpi_ssm_get_data (transfer->ssm))->name, error);
  fpi_ssm_usb_transfer_cb (transfer, dev, data, error);
}

static void
cmd_read_cb (FpiUsbTransfer *transfer, FpDevice *dev, gpointer data, GError *error)
{
  FpiDeviceElan0c4d *self = FPI_DEVICE_ELAN0C4D (dev);
  FpiSsm *ssm = transfer->ssm;
  const ElanCmd *cmd = fpi_ssm_get_data (ssm);

  if (error == NULL && (gsize) transfer->actual_length < cmd->in_min)
    error = g_error_new (ELAN_ERROR, ELAN_ERROR_PROTOCOL, "%s: short answer %zd/%zu",
                         cmd->name, transfer->actual_length, cmd->in_min);
  if (error == NULL)
    {
      self->resp_len = MIN ((gsize) transfer->actual_length, sizeof self->resp);
      memcpy (self->resp, transfer->buffer, self->resp_len);
      elan_log_hex ("in ", cmd->ep_in, self->resp, self->resp_len);
      if (cmd->check && !cmd->check (self, &error) && error == NULL)
        error = g_error_new (ELAN_ERROR, ELAN_ERROR_PROTOCOL, "%s: answer rejected", cmd->name);
    }
  if (error)
    {
      elan_log_error (cmd->name, error);
      fpi_ssm_mark_failed (ssm, error);
      return;
    }
  fpi_ssm_mark_completed (ssm);
}

static void
cmd_handler (FpiSsm *ssm, FpDevice *dev)
{
  const ElanCmd *cmd = fpi_ssm_get_data (ssm);
  g_autoptr(FpiUsbTransfer) transfer = fpi_usb_transfer_new (dev);

  transfer->ssm = ssm;
  if (fpi_ssm_get_cur_state (ssm) == CMD_WRITE)
    {
      if (elan_cmd_is_forbidden (cmd->out, cmd->out_len))
        {
          fpi_ssm_mark_failed (ssm, g_error_new (ELAN_ERROR, ELAN_ERROR_FORBIDDEN,
                                                 "%s: forbidden command refused", cmd->name));
          return;
        }
      elan_log_hex ("out", ELAN_EP_OUT, cmd->out, cmd->out_len);
      transfer->short_is_error = TRUE;
      fpi_usb_transfer_fill_bulk_full (transfer, ELAN_EP_OUT, g_memdup2 (cmd->out, cmd->out_len),
                                       cmd->out_len, g_free);
      fpi_usb_transfer_submit (g_steal_pointer (&transfer), ELAN_WRITE_TIMEOUT_MS, NULL, cmd_write_cb, NULL);
    }
  else if (cmd->in_min == 0)
    fpi_ssm_mark_completed (ssm);
  else
    {
      fpi_usb_transfer_fill_bulk (transfer, cmd->ep_in, cmd->in_max);
      fpi_usb_transfer_submit (g_steal_pointer (&transfer), cmd->timeout_ms,
                               cmd->cancellable ? fpi_device_get_cancellable (dev) : NULL,
                               cmd_read_cb, NULL);
    }
}

FpiSsm *
elan_cmd_ssm_new (FpDevice *dev, gconstpointer arg)
{
  FpiSsm *ssm = fpi_ssm_new_full (dev, cmd_handler, CMD_STATES, CMD_STATES, "elan-cmd");

  fpi_ssm_set_data (ssm, g_memdup2 (arg, sizeof (ElanCmd)), g_free);
  return ssm;
}
