/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* Open (initialize + calibrate) and its recovery paths against the simulated sensor. */
#include <gusb.h>
#include "sim.h"

static const SimImage no_images[] = { SIM_EMPTY };
static const guint8 no_press[] = { 0xAF };

static GError *
run_open (FpiDeviceElan0c4d *dev)
{
  return sim_run (elan_retry_ssm_new (FP_DEVICE (dev), "open", elan_reinit_ssm_new, NULL, TRUE));
}

#define SENSOR(...) SimSensor s = { .images = no_images, .n_images = 1, .press = no_press, .n_press = 1, __VA_ARGS__ }

static void
test_open (void)
{
  SENSOR (.ready_after = 1);
  FpiDeviceElan0c4d *dev = sim_start (&s);
  g_autoptr(GError) error = run_open (dev);

  g_assert_no_error (error);
  g_assert_cmpstr (s.wire->str, ==, W_INIT " 4024 4023 4023");
  g_assert_cmpuint (sim_warnings (), ==, 0);
  sim_end (dev);
}

/* A timeout cancels (00 0b), re-initializes and tries the whole operation again. */
static void
test_timeout_recovery (void)
{
  SENSOR (.fault_cmd = { 0x9a, 0x10 }, .fault_count = 1,
          .fault_domain = G_USB_DEVICE_ERROR, .fault_code = G_USB_DEVICE_ERROR_TIMED_OUT);
  FpiDeviceElan0c4d *dev = sim_start (&s);
  g_autoptr(GError) error = run_open (dev);

  g_assert_no_error (error);
  g_assert_cmpstr (s.wire->str, ==,
                   "996001 000b 9a10 000b " W_INIT " " W_INIT " " W_CAL);
  g_assert_cmpuint (sim_warnings (), >=, 1);
  sim_end (dev);
}

/* An I/O error releases and claims the interface again instead of sending 00 0b. */
static void
test_io_error_reopens (void)
{
  SENSOR (.fault_cmd = { 0x9a, 0x10 }, .fault_count = 1,
          .fault_domain = G_USB_DEVICE_ERROR, .fault_code = G_USB_DEVICE_ERROR_IO);
  FpiDeviceElan0c4d *dev = sim_start (&s);
  g_autoptr(GError) error = run_open (dev);

  g_assert_no_error (error);
  g_assert_cmpstr (s.wire->str, ==,
                   "996001 000b 9a10 [release] [claim] " W_INIT " " W_INIT " " W_CAL);
  sim_end (dev);
}

/* A vanished device is never retried: one attempt, the error reaches the caller. */
static void
test_no_device_is_final (void)
{
  SENSOR (.fault_cmd = { 0x9a, 0x10 }, .fault_count = 1,
          .fault_domain = G_USB_DEVICE_ERROR, .fault_code = G_USB_DEVICE_ERROR_NO_DEVICE);
  FpiDeviceElan0c4d *dev = sim_start (&s);
  g_autoptr(GError) error = run_open (dev);

  g_assert_error (error, G_USB_DEVICE_ERROR, G_USB_DEVICE_ERROR_NO_DEVICE);
  g_assert_cmpstr (s.wire->str, ==, "996001 000b 9a10");
  g_assert_cmpuint (sim_warnings (), >=, 1);
  sim_end (dev);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);
  g_test_add_func ("/sim/open", test_open);
  g_test_add_func ("/sim/open-timeout-recovery", test_timeout_recovery);
  g_test_add_func ("/sim/open-io-error-reopens", test_io_error_reopens);
  g_test_add_func ("/sim/open-no-device-is-final", test_no_device_is_final);
  return g_test_run ();
}
