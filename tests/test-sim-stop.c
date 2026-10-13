/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* Ending a wait: cancellation and a deadline abort with 00 0b; without a deadline it re-arms. */
#include <string.h>
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
  /* A loaded machine may cancel before 40 3f goes out: the commands must not depend on when. */
  g_assert_nonnull (strstr (s.wire->str, "[cancel]"));
  g_string_replace (s.wire, "[cancel] ", "", 1);
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

static void
test_waits_until_a_finger_comes (void)
{
  static const SimImage late[] = { SIM_EMPTY, SIM_EMPTY, SIM_EMPTY, SIM_FINGER };
  SimSensor s = { .images = late, .n_images = 4, .press = ff, .n_press = 1 };
  FpiDeviceElan0c4d *dev = sim_start (&s);
  g_autoptr(GError) error = NULL;

  dev->finger_window_ms = 120;  /* silent slices end and re-arm instead of failing */
  error = sim_run (elan_capture_ssm_new (FP_DEVICE (dev)));
  g_assert_no_error (error);
  g_assert_cmpint (dev->assessment.kind, ==, ELAN_FRAME_FINGER);
  g_assert_cmpuint (sim_warnings (), ==, 0);
  sim_end (dev);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);
  g_test_add_func ("/sim/stop/cancel-aborts-the-wait", test_cancel_aborts_the_wait);
  g_test_add_func ("/sim/stop/no-finger-in-time", test_no_finger_in_time);
  g_test_add_func ("/sim/stop/waits-until-a-finger-comes", test_waits_until_a_finger_comes);
  return g_test_run ();
}
