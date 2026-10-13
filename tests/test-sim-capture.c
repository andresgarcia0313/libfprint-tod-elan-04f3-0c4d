/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* Capture flow against the simulated sensor: wire sequences, recoveries and limits. */
#include "sim.h"

static const SimImage rest[] = { SIM_EMPTY, SIM_FINGER };
static const SimImage empty_frame[] = { SIM_EMPTY, SIM_EMPTY, SIM_FINGER };
static const SimImage bad_frames[] = { SIM_EMPTY, SIM_ZEROS, SIM_SATURATED, SIM_FINGER };
static const guint8 ff_af[] = { 0xFF, 0xAF }, cc_af[] = { 0xCC, 0xAF }, ff[] = { 0xFF };

#define CAPTURE(sensor, ...) \
  SimSensor sensor = { __VA_ARGS__ }; \
  FpiDeviceElan0c4d *dev = sim_start (&sensor); \
  g_autoptr(GError) error = sim_run (elan_capture_ssm_new (FP_DEVICE (dev)))

static void
test_finger_resting (void)
{
  CAPTURE (s, .images = rest, .n_images = 2, .press = ff, .n_press = 1);

  g_assert_no_error (error);
  g_assert_cmpstr (s.wire->str, ==, W_PREPARE " 0009");
  g_assert_cmpint (dev->assessment.kind, ==, ELAN_FRAME_FINGER);
  sim_end (dev);
}

/* The image arrives in 4000 byte reads and is resumed from what was received. */
static void
test_short_image_reads (void)
{
  CAPTURE (s, .images = rest, .n_images = 2, .press = ff, .n_press = 1, .image_chunk = 4000);

  g_assert_no_error (error);
  g_assert_cmpstr (s.wire->str, ==, W_PREPARE " 0009");
  g_assert_cmpfloat (dev->assessment.coverage, ==, 1.0);
  sim_end (dev);
}

/* 0xFF means "no data": the command is sent again, never read on. */
static void
test_wait_resends_on_ff (void)
{
  CAPTURE (s, .images = empty_frame, .n_images = 3, .press = ff_af, .n_press = 2);

  g_assert_no_error (error);
  g_assert_cmpstr (s.wire->str, ==, W_PREPARE " 0009 " W_SSO " 403e 4031 403f 403f 0009");
  sim_end (dev);
}

static void
test_invalid_frames_recalibrate_then_reinitialize (void)
{
  CAPTURE (s, .images = bad_frames, .n_images = 4, .press = ff, .n_press = 1);

  g_assert_no_error (error);
  g_assert_cmpstr (s.wire->str, ==,
                   W_PREPARE " 0009 " W_CAL " 0009 " W_INIT " " W_CAL " 0009");
  g_assert_cmpuint (sim_warnings (), ==, 0);  /* a recovered frame is not worth a journal warning */
  sim_end (dev);
}

static void
test_key_lost_rekeys (void)
{
  CAPTURE (s, .images = empty_frame, .n_images = 3, .press = cc_af, .n_press = 2);

  g_assert_no_error (error);
  g_assert_cmpstr (s.wire->str, ==, W_PREPARE " 0009 " W_SSO " 403e 4031 403f " W_INIT " 403f 0009");
  sim_end (dev);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);
  g_test_add_func ("/sim/capture/finger-resting", test_finger_resting);
  g_test_add_func ("/sim/capture/short-image-reads", test_short_image_reads);
  g_test_add_func ("/sim/capture/wait-resends-on-ff", test_wait_resends_on_ff);
  g_test_add_func ("/sim/capture/invalid-frames", test_invalid_frames_recalibrate_then_reinitialize);
  g_test_add_func ("/sim/capture/key-lost-rekeys", test_key_lost_rekeys);
  return g_test_run ();
}
