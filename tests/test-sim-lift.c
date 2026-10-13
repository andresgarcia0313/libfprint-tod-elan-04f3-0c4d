/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* Waiting for the finger to lift between touches, and cancelling an enroll half way. */
#include "sim.h"
#include "elan0c4d-enroll.h"

static const SimImage held[] = { SIM_FINGER, SIM_FINGER, SIM_EMPTY };
static const guint8 ff[] = { 0xFF }, block[] = { SIM_BLOCK };

/* Frames are read every ELAN_LIFT_POLL_MS while the finger stays; nothing else is sent. */
static void
test_waits_until_the_finger_goes (void)
{
  SimSensor s = { .images = held, .n_images = 3, .press = ff, .n_press = 1 };
  FpiDeviceElan0c4d *dev = sim_start (&s);
  gint64 start = g_get_monotonic_time ();
  g_autoptr(GError) error = sim_run (elan_lift_ssm_new (FP_DEVICE (dev), NULL));

  g_assert_no_error (error);
  g_assert_cmpstr (s.wire->str, ==, "0009 0009 0009");
  g_assert_cmpint (g_get_monotonic_time () - start, >=, 2 * ELAN_LIFT_POLL_MS * 1000);
  g_assert_cmpint (dev->assessment.kind, ==, ELAN_FRAME_EMPTY);
  sim_end (dev);
}

static void
test_finger_not_lifted_in_time (void)
{
  SimSensor s = { .images = held, .n_images = 1, .press = ff, .n_press = 1 };
  FpiDeviceElan0c4d *dev = sim_start (&s);
  g_autoptr(GError) error = NULL;

  dev->capture_timeout_ms = 400;
  error = sim_run (elan_lift_ssm_new (FP_DEVICE (dev), NULL));
  g_assert_error (error, ELAN_ERROR, ELAN_ERROR_TIMEOUT);
  g_assert_nonnull (strstr (error->message, "finger not lifted"));
  sim_end (dev);
}

/* Without a deadline the finger that never goes is waited for until fprintd cancels. */
static void
test_cancel_while_the_finger_stays (void)
{
  static const SimImage stays[] = { SIM_EMPTY, SIM_FINGER };
  SimSensor s = { .images = stays, .n_images = 2, .press = ff, .n_press = 1 };
  FpiDeviceElan0c4d *dev = sim_start (&s);
  g_autoptr(GError) error = NULL;

  sim_cancel_in (500);
  error = sim_run (elan_stages_ssm_new (FP_DEVICE (dev)));
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
  sim_assert_progress (1, G_MAXUINT, -1);
  g_assert_true (g_str_has_prefix (s.wire->str, W_PREPARE " 0009 0009 0009 0009"));
  g_assert_true (g_str_has_suffix (s.wire->str, " 0009 [cancel]"));  /* no 00 09 after it, no 00 0b needed */
  g_assert_cmpuint (sim_warnings (), ==, 0);
  sim_end (dev);
}

/* Cancelled while the second touch is awaited (40 3f): the wait is aborted with 00 0b. */
static void
test_cancel_before_the_second_touch (void)
{
  static const SimImage touches[] = { SIM_FINGER, SIM_EMPTY };
  SimSensor s;
  FpiDeviceElan0c4d *dev = sim_start_touches (&s, touches, 2);
  g_autoptr(GError) error = NULL;

  s.press = block;
  sim_cancel_in (300);
  error = sim_run (elan_stages_ssm_new (FP_DEVICE (dev)));
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_CANCELLED);
  sim_assert_progress (1, G_MAXUINT, -1);
  g_assert_true (g_str_has_suffix (s.wire->str, " 0009 " W_SSO " 403e 4031 403f [cancel] 000b"));
  g_assert_cmpuint (dev->enrollment->n, ==, 1);
  sim_end (dev);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);
  g_test_add_func ("/sim/lift/waits-until-the-finger-goes", test_waits_until_the_finger_goes);
  g_test_add_func ("/sim/lift/finger-not-lifted-in-time", test_finger_not_lifted_in_time);
  g_test_add_func ("/sim/lift/cancel-while-the-finger-stays", test_cancel_while_the_finger_stays);
  g_test_add_func ("/sim/lift/cancel-before-the-second-touch", test_cancel_before_the_second_touch);
  return g_test_run ();
}
