/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * prepare_capture (elan0c4d.py): cancel, SSO on, image info, session check,
 * calibration, background image, delete the pre-captured image. The background
 * is kept for the matcher; the classifier never uses it.
 */
#include "elan0c4d-ops.h"

enum { PREP_SCRIPT, PREP_CALIBRATE, PREP_IMAGE, PREP_BACKGROUND, PREP_DELETE, PREP_END, PREP_STATES };

static gboolean
check_session (FpiDeviceElan0c4d *self, GError **error)
{
  if (self->resp[3] == 0x01 || self->resp[3] == 0xFF)
    return TRUE;
  g_set_error (error, ELAN_ERROR, ELAN_ERROR_KEY_LOST, "session status 0x%02x", self->resp[3]);
  return FALSE;
}

static const ElanCmd prepare_script[] = {
  ELAN_WRITE ("cancel", elan_cmd_cancel),
  ELAN_WRITE ("sso-on", elan_cmd_sso_on),
  ELAN_WRITE ("image-info", elan_cmd_recognize),
  ELAN_QUERY ("precapture-query", elan_cmd_precapture, 4, NULL),
  ELAN_QUERY ("session-status", elan_cmd_session_status, 4, check_session),
  ELAN_WRITE ("session-bypass", elan_cmd_session),
};
static const ElanCmd delete_precapture = ELAN_WRITE ("precapture-delete", elan_cmd_precapture_del);

static void
prepare_handler (FpiSsm *ssm, FpDevice *dev)
{
  FpiDeviceElan0c4d *self = FPI_DEVICE_ELAN0C4D (dev);

  switch (fpi_ssm_get_cur_state (ssm))
    {
    case PREP_SCRIPT:
      fpi_ssm_start_subsm (ssm, elan_script_ssm_new (dev, prepare_script, G_N_ELEMENTS (prepare_script)));
      break;

    case PREP_CALIBRATE:
      fpi_ssm_start_subsm (ssm, elan_calibrate_ssm_new (dev, NULL));
      break;

    case PREP_IMAGE:
      fpi_ssm_start_subsm (ssm, elan_image_ssm_new (dev, NULL));
      break;

    case PREP_BACKGROUND:
      memcpy (self->background, self->frame, sizeof self->background);
      g_info ("background taken (%d samples)", ELAN_PIXELS);
      fpi_ssm_next_state (ssm);
      break;

    case PREP_DELETE:
      fpi_ssm_start_subsm (ssm, elan_cmd_ssm_new (dev, &delete_precapture));
      break;

    default:
      fpi_ssm_mark_completed (ssm);
    }
}

FpiSsm *
elan_prepare_ssm_new (FpDevice *dev, gconstpointer arg)
{
  return fpi_ssm_new_full (dev, prepare_handler, PREP_STATES, PREP_STATES, "elan-prepare");
}
