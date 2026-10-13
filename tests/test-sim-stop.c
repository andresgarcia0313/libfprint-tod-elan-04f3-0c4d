/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* Ending a wait: user cancellation and the capture deadline both abort with 00 0b. */
#include "sim.h"

static const SimImage empty_frame[] = { SIM_EMPTY, SIM_EMPTY, SIM_FINGER };
static const guint8 block[] = { SIM_BLOCK }, ff[] = { 0xFF };

static void
test_cancel_aborts_the_wait (void)
{
  SimSensor s = { .images = empty_frame, .n_images = 3, .press = block, .n_press = 1 };
  FpiDeviceElan0c4d *dev = sim_start (&s);
  g_autoptr(GError) error = NULL;

  sim_cancel_in (150);
  error = sim_run (elan_capture_ssm_new (FP_DEVICE (dev)));
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
  g_assert_cmpstr (s.wire->str, ==, W_PREPARE " 0009 " W_SSO " 403e 4031 403f 000b");
  sim_end (dev);
}

static void
test_no_finger_in_time (void)
{
  SimSensor s = { .images = empty_frame, .n_images = 2, .press = ff, .n_press = 1 };
  FpiDeviceElan0c4d *dev = sim_start (&s);
  g_autoptr(GError) error = NULL;

  dev->capture_timeout_ms = 400;
  error = sim_run (elan_capture_ssm_new (FP_DEVICE (dev)));
  g_assert_error (error, ELAN_ERROR, ELAN_ERROR_TIMEOUT);
  g_assert_nonnull (strstr (error->message, "no usable finger frame"));
  g_assert_true (g_str_has_suffix (s.wire->str, " 403f 000b"));
  sim_end (dev);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);
  g_test_add_func ("/sim/stop/cancel-aborts-the-wait", test_cancel_aborts_the_wait);
  g_test_add_func ("/sim/stop/no-finger-in-time", test_no_finger_in_time);
  return g_test_run ();
}
