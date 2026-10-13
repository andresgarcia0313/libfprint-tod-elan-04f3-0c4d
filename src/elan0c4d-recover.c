/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Remedies (elan_session.py _recover): timeouts and protocol errors cancel
 * the pending operation (00 0b); I/O errors release and claim the interface
 * again; a lost key only needs the 99 60 01 that initialization sends first.
 * Every remedy ends re-initializing the sensor.
 */
#include <gusb.h>
#include "elan0c4d-ops.h"

enum { RECOVER_PRE, RECOVER_INIT, RECOVER_END, RECOVER_STATES };

static gboolean
reclaim_interface (FpDevice *dev, GError **error)
{
  GUsbDevice *usb = fpi_device_get_usb_device (dev);
  g_autoptr(GError) ignored = NULL;

  if (!g_usb_device_release_interface (usb, 0, 0, &ignored))
    g_info ("release before reclaim ignored: %s", ignored->message);
  return g_usb_device_claim_interface (usb, 0, 0, error);
}

static void
recover_handler (FpiSsm *ssm, FpDevice *dev)
{
  ElanRemedy remedy = GPOINTER_TO_INT (fpi_ssm_get_data (ssm));
  g_autoptr(GError) error = NULL;

  switch (fpi_ssm_get_cur_state (ssm))
    {
    case RECOVER_PRE:
      if (remedy == ELAN_REMEDY_REINIT)
        fpi_ssm_start_subsm (ssm, elan_cmd_ssm_new (dev, &elan_cancel_cmd));
      else if (remedy == ELAN_REMEDY_REOPEN && !reclaim_interface (dev, &error))
        fpi_ssm_mark_failed (ssm, g_steal_pointer (&error));
      else
        fpi_ssm_next_state (ssm);
      break;

    case RECOVER_INIT:
      fpi_ssm_start_subsm (ssm, elan_init_ssm_new (dev, NULL));
      break;

    default:
      fpi_ssm_mark_completed (ssm);
    }
}

FpiSsm *
elan_recover_ssm_new (FpDevice *dev, ElanRemedy remedy)
{
  FpiSsm *ssm = fpi_ssm_new_full (dev, recover_handler, RECOVER_STATES, RECOVER_STATES, "elan-recover");

  fpi_ssm_set_data (ssm, GINT_TO_POINTER (remedy), NULL);
  return ssm;
}
