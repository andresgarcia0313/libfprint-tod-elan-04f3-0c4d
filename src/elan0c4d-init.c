/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Initialization and calibration (elan0c4d.py initialize/calibrate). Mirrors
 * FpDeviceInitialize of the vendor driver on the non-SGX route: session
 * bypass, cancel, pre-capture query, sensor size, versions, image info.
 */
#include "elan0c4d-ops.h"

static gboolean
check_size (FpiDeviceElan0c4d *self, guint expected, const char *what, GError **error)
{
  guint got = self->resp[0] + 1;

  if (got == expected)
    return TRUE;
  g_set_error (error, ELAN_ERROR, ELAN_ERROR_PROTOCOL,
               "unexpected sensor %s %u (expected %u)", what, got, expected);
  return FALSE;
}

static gboolean
check_height (FpiDeviceElan0c4d *self, GError **error)
{
  return check_size (self, ELAN_HEIGHT, "height", error);
}

static gboolean
check_width (FpiDeviceElan0c4d *self, GError **error)
{
  return check_size (self, ELAN_WIDTH, "width", error);
}

static gboolean
log_firmware (FpiDeviceElan0c4d *self, GError **error)
{
  g_info ("bridge firmware version 0x%02x%02x, sensor %ux%u", self->resp[0], self->resp[1],
          ELAN_WIDTH, ELAN_HEIGHT);
  return TRUE;
}

static const ElanCmd init_script[] = {
  ELAN_WRITE ("session-bypass", elan_cmd_session),
  ELAN_WRITE ("cancel", elan_cmd_cancel),
  ELAN_QUERY ("precapture-query", elan_cmd_precapture, 4, NULL),
  ELAN_QUERY ("sensor-height", elan_cmd_height, 1, check_height),
  ELAN_QUERY ("sensor-width", elan_cmd_width, 1, check_width),
  ELAN_QUERY ("firmware-version", elan_cmd_fw_version, 2, log_firmware),
  ELAN_QUERY ("sample-version", elan_cmd_sample_version, 2, NULL),
  ELAN_WRITE ("image-info", elan_cmd_recognize),
};

FpiSsm *
elan_init_ssm_new (FpDevice *dev, gconstpointer arg)
{
  return elan_script_ssm_new (dev, init_script, G_N_ELEMENTS (init_script));
}

static void
reinit_handler (FpiSsm *ssm, FpDevice *dev)
{
  switch (fpi_ssm_get_cur_state (ssm))
    {
    case 0:
      fpi_ssm_start_subsm (ssm, elan_init_ssm_new (dev, NULL));
      break;

    case 1:
      fpi_ssm_start_subsm (ssm, elan_calibrate_ssm_new (dev, NULL));
      break;

    default:
      fpi_ssm_mark_completed (ssm);
    }
}

FpiSsm *
elan_reinit_ssm_new (FpDevice *dev, gconstpointer arg)
{
  return fpi_ssm_new_full (dev, reinit_handler, 3, 3, "elan-reinit");
}
