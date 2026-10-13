/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Arming the finger detector (elan_capture.py _arm): SSO on, wait for the
 * finger to be released (a timeout only means one is resting: the arm ends and
 * the caller reads the frame directly), LED, then wait for 0xAF/0x55 until
 * the capture deadline.
 */
#include "elan0c4d-ops.h"

enum { ARM_SSO, ARM_RELEASE, ARM_GPIO, ARM_PRESS, ARM_STATES };

static const guint8 finger_off[] = { ELAN_CODE_FINGER_OFF };
static const guint8 finger_on[] = { ELAN_CODE_FINGER_A, ELAN_CODE_FINGER_B };
static const ElanCmd sso_on_cmd = ELAN_WRITE ("sso-on", elan_cmd_sso_on);
static const ElanCmd gpio_low_cmd = ELAN_WRITE ("gpio-low", elan_cmd_gpio_low);
static const ElanWait release_wait = { "wait-release", elan_cmd_wait_release, sizeof elan_cmd_wait_release,
                                       finger_off, sizeof finger_off, ELAN_RELEASE_WAIT_MS };

gint64
elan_capture_left_ms (const FpiDeviceElan0c4d *self)
{
  return (self->deadline_us - g_get_monotonic_time ()) / 1000;
}

GError *
elan_no_finger_error (const FpiDeviceElan0c4d *self)
{
  return g_error_new (ELAN_ERROR, ELAN_ERROR_TIMEOUT, "no usable finger frame in %d ms (last: %s)",
                      self->capture_timeout_ms, elan_frame_kind_name (self->assessment.kind));
}

static void
release_done (FpiSsm *ssm, GError *error)
{
  if (error && elan_error_is_timeout (error))
    {
      g_info ("finger still on the sensor; reading it directly");
      g_error_free (error);
      fpi_ssm_mark_completed (ssm);
    }
  else if (error)
    fpi_ssm_mark_failed (ssm, error);
  else
    fpi_ssm_next_state (ssm);
}

static void
press_done (FpiSsm *ssm, GError *error)
{
  if (error && elan_error_is_timeout (error))
    {
      g_error_free (error);
      error = elan_no_finger_error (FPI_DEVICE_ELAN0C4D (fpi_ssm_get_device (ssm)));
    }
  if (error)
    fpi_ssm_mark_failed (ssm, error);
  else
    fpi_ssm_mark_completed (ssm);
}

static void
arm_handler (FpiSsm *ssm, FpDevice *dev)
{
  ElanWait *press = fpi_ssm_get_data (ssm);

  switch (fpi_ssm_get_cur_state (ssm))
    {
    case ARM_SSO:
      fpi_ssm_start_subsm (ssm, elan_retry_ssm_new (dev, "sso-on", elan_cmd_ssm_new, &sso_on_cmd, TRUE));
      break;

    case ARM_RELEASE:
      elan_try (ssm, elan_retry_ssm_new (dev, "wait-release", elan_wait_ssm_new, &release_wait, FALSE),
                release_done);
      break;

    case ARM_GPIO:
      fpi_ssm_start_subsm (ssm, elan_retry_ssm_new (dev, "gpio-low", elan_cmd_ssm_new, &gpio_low_cmd, TRUE));
      break;

    default:
      *press = (ElanWait) { "wait-finger", elan_cmd_wait_press, sizeof elan_cmd_wait_press, finger_on,
                            sizeof finger_on, (guint) MAX (elan_capture_left_ms (FPI_DEVICE_ELAN0C4D (dev)), 1) };
      elan_try (ssm, elan_retry_ssm_new (dev, "wait-finger", elan_wait_ssm_new, press, FALSE), press_done);
    }
}

FpiSsm *
elan_arm_ssm_new (FpDevice *dev, gconstpointer arg)
{
  FpiSsm *ssm = fpi_ssm_new_full (dev, arm_handler, ARM_STATES, ARM_STATES, "elan-arm");

  fpi_ssm_set_data (ssm, g_new0 (ElanWait, 1), g_free);
  return ssm;
}
