/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* Refused commands and the 20-read limit of the image, against the simulated sensor. */
#include "sim.h"

static const SimImage empty[] = { SIM_EMPTY };
static const guint8 press[] = { 0xAF };

/* 42 01 "RUNIAP": it must never reach the wire, whoever builds it. */
static void
test_forbidden_command_is_refused (void)
{
  static const guint8 runiap[] = { 0x42, 0x01, 0x52, 0x55, 0x4e, 0x49, 0x41, 0x50 };
  const ElanCmd bad = ELAN_WRITE ("runiap", runiap);
  SimSensor s = { .images = empty, .n_images = 1, .press = press, .n_press = 1 };
  FpiDeviceElan0c4d *dev = sim_start (&s);
  g_autoptr(GError) error = sim_run (elan_cmd_ssm_new (FP_DEVICE (dev), &bad));

  g_assert_error (error, ELAN_ERROR, ELAN_ERROR_FORBIDDEN);
  g_assert_cmpstr (s.wire->str, ==, "");
  sim_end (dev);
}

/* Reads of 100 bytes never complete 15600 in 20 reads: error after 3 attempts, no endless loop. */
static void
test_image_read_limit (void)
{
  SimSensor s = { .images = empty, .n_images = 1, .press = press, .n_press = 1, .image_chunk = 100 };
  FpiDeviceElan0c4d *dev = sim_start (&s);
  g_autoptr(GError) error =
    sim_run (elan_retry_ssm_new (FP_DEVICE (dev), "image", elan_image_ssm_new, NULL, TRUE));

  g_assert_error (error, ELAN_ERROR, ELAN_ERROR_EXHAUSTED);
  g_assert_nonnull (strstr (error->message, "short image"));
  sim_end (dev);
}

/* Calibration that never reaches 0x03: 3 attempts of 50 polls each, then a clear error. */
static void
test_calibration_never_ready (void)
{
  SimSensor s = { .ready_after = G_MAXUINT, .images = empty, .n_images = 1, .press = press, .n_press = 1 };
  FpiDeviceElan0c4d *dev = sim_start (&s);
  g_autoptr(GError) error =
    sim_run (elan_retry_ssm_new (FP_DEVICE (dev), "open", elan_reinit_ssm_new, NULL, TRUE));
  g_auto(GStrv) tokens = g_strsplit (s.wire->str, " ", -1);
  guint polls = 0;

  g_assert_error (error, ELAN_ERROR, ELAN_ERROR_EXHAUSTED);
  for (guint i = 0; tokens[i]; i++)
    polls += g_str_equal (tokens[i], "4023");
  g_assert_cmpuint (polls, ==, 3 * ELAN_CALIBRATION_POLLS);
  sim_end (dev);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);
  g_test_add_func ("/sim/limits/forbidden-command-is-refused", test_forbidden_command_is_refused);
  g_test_add_func ("/sim/limits/image-read-limit", test_image_read_limit);
  g_test_add_func ("/sim/limits/calibration-never-ready", test_calibration_never_ready);
  return g_test_run ();
}
