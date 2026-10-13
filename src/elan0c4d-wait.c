/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * SensorWaitFingerPress / Release (elan_wait.py). After 40 3f / 40 3e the MCU
 * answers with status codes on EP 0x83. Reading stops at the first decisive
 * answer: a bulk IN left pending on 0x83 makes the MCU reset. An answer with
 * no decisive code (0xFF = "no data") re-sends the command, never reads on.
 * Timeout or cancellation abort the wait with 00 0b before failing.
 */
#include "elan0c4d-codes.h"
#include "elan0c4d-wait.h"

static void
wait_free (gpointer data)
{
  Wait *wait = data;

  g_clear_error (&wait->pending);
  g_free (wait);
}

static void
wait_scan (FpiSsm *ssm, FpiDeviceElan0c4d *self, const Wait *wait)
{
  switch (elan_scan_codes (self->resp, self->resp_len, wait->w.wanted, wait->w.n_wanted,
                           &self->wait_code))
    {
    case ELAN_SCAN_FOUND:
      g_info ("%s: code 0x%02x", wait->w.name, self->wait_code);
      fpi_ssm_mark_completed (ssm);
      break;

    case ELAN_SCAN_KEY_LOST:
      fpi_ssm_mark_failed (ssm, g_error_new (ELAN_ERROR, ELAN_ERROR_KEY_LOST,
                                             "%s: sensor reported 0xCC", wait->w.name));
      break;

    default:
      g_debug ("%s: no decisive code in %zu bytes; re-sending the command", wait->w.name, self->resp_len);
      fpi_ssm_jump_to_state_delayed (ssm, WAIT_SEND, ELAN_RESEND_MS);
    }
}

static void
aborted (FpiSsm *ssm, GError *error)
{
  Wait *wait = fpi_ssm_get_data (ssm);

  if (error)
    {
      g_warning ("cancel after %s failed: %s", wait->w.name, error->message);
      g_error_free (error);
    }
  fpi_ssm_mark_failed (ssm, g_steal_pointer (&wait->pending));
}

static void
wait_handler (FpiSsm *ssm, FpDevice *dev)
{
  Wait *wait = fpi_ssm_get_data (ssm);
  gint64 left_ms = (wait->deadline_us - g_get_monotonic_time ()) / 1000;
  ElanCmd send = { wait->w.name, wait->w.cmd, wait->w.cmd_len, 0, 0, 0, 0, FALSE, NULL };
  int state = fpi_ssm_get_cur_state (ssm);

  if (left_ms <= 0 && state <= WAIT_READ)
    {
      wait->pending = elan_wait_timeout_error (wait);
      state = WAIT_ABORT;
    }
  if (state == WAIT_SEND)
    fpi_ssm_start_subsm (ssm, elan_cmd_ssm_new (dev, &send));
  else if (state == WAIT_READ)
    elan_wait_read (ssm, dev, left_ms);
  else if (state == WAIT_SCAN)
    wait_scan (ssm, FPI_DEVICE_ELAN0C4D (dev), wait);
  else
    elan_try (ssm, elan_cmd_ssm_new (dev, &elan_cancel_cmd), aborted);
}

FpiSsm *
elan_wait_ssm_new (FpDevice *dev, gconstpointer arg)
{
  FpiSsm *ssm = fpi_ssm_new_full (dev, wait_handler, WAIT_STATES, WAIT_STATES, "elan-wait");
  Wait *wait = g_new0 (Wait, 1);

  wait->w = *(const ElanWait *) arg;
  wait->deadline_us = g_get_monotonic_time () + (gint64) wait->w.timeout_ms * 1000;
  fpi_ssm_set_data (ssm, wait, wait_free);
  return ssm;
}
