/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Calibration (elan0c4d.py calibrate): read the bridge mean (40 24), then poll
 * 40 23 every 10 ms until its first byte is 0x03 ("Calibration Ready"), at most 50 times.
 */
#include "elan0c4d-ops.h"

static const ElanCmd calib_mean_cmd = ELAN_QUERY ("calibration-mean", elan_cmd_calib_mean, 2, NULL);
static const ElanCmd calib_start_cmd = ELAN_QUERY ("calibration-start", elan_cmd_calib_start, 1, NULL);

enum { CAL_MEAN, CAL_POLL, CAL_CHECK, CAL_STATES };

static void
calibrate_handler (FpiSsm *ssm, FpDevice *dev)
{
  FpiDeviceElan0c4d *self = FPI_DEVICE_ELAN0C4D (dev);
  guint *polls = fpi_ssm_get_data (ssm);

  switch (fpi_ssm_get_cur_state (ssm))
    {
    case CAL_MEAN:
      fpi_ssm_start_subsm (ssm, elan_cmd_ssm_new (dev, &calib_mean_cmd));
      break;

    case CAL_POLL:
      (*polls)++;
      fpi_ssm_start_subsm (ssm, elan_cmd_ssm_new (dev, &calib_start_cmd));
      break;

    default:
      if (self->resp[0] == ELAN_CALIBRATION_READY)
        {
          g_info ("calibration ready after %u polls", *polls);
          fpi_ssm_mark_completed (ssm);
        }
      else if (*polls >= ELAN_CALIBRATION_POLLS)
        fpi_ssm_mark_failed (ssm, g_error_new (ELAN_ERROR, ELAN_ERROR_CALIBRATION,
                                               "calibration never reached 0x03 (%u polls)", *polls));
      else
        fpi_ssm_jump_to_state_delayed (ssm, CAL_POLL, ELAN_CALIBRATION_POLL_MS);
    }
}

FpiSsm *
elan_calibrate_ssm_new (FpDevice *dev, gconstpointer arg)
{
  FpiSsm *ssm = fpi_ssm_new_full (dev, calibrate_handler, CAL_STATES, CAL_STATES, "elan-calibrate");

  fpi_ssm_set_data (ssm, g_new0 (guint, 1), g_free);
  return ssm;
}
