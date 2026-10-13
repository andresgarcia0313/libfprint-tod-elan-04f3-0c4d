/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* Forbidden commands, device revision and status codes: no hardware involved. */
#include <glib.h>
#include "elan0c4d-codes.h"
#include "elan0c4d-guard.h"
#include "elan0c4d-proto.h"

static void
test_forbidden_commands (void)
{
  const guint8 runiap[] = { 0x42, 0x01, 0x52, 0x55, 0x4e, 0x49, 0x41, 0x50 };
  const guint8 wdtrst[] = { 0x40, 0x27, 0x57, 0x44, 0x54, 0x52, 0x53, 0x54 };
  const guint8 near_a[] = { 0x42, 0x02 }, near_b[] = { 0x40, 0x28 }, one[] = { 0x42 };

  g_assert_true (elan_cmd_is_forbidden (runiap, sizeof runiap));
  g_assert_true (elan_cmd_is_forbidden (runiap, 2));
  g_assert_true (elan_cmd_is_forbidden (wdtrst, sizeof wdtrst));
  g_assert_false (elan_cmd_is_forbidden (near_a, sizeof near_a));
  g_assert_false (elan_cmd_is_forbidden (near_b, sizeof near_b));
  g_assert_false (elan_cmd_is_forbidden (one, sizeof one));
  g_assert_false (elan_cmd_is_forbidden (NULL, 0));
}

#define ALLOWED(cmd) g_assert_false (elan_cmd_is_forbidden (cmd, sizeof cmd))

static void
test_driver_commands_are_allowed (void)
{
  ALLOWED (elan_cmd_session); ALLOWED (elan_cmd_cancel); ALLOWED (elan_cmd_precapture);
  ALLOWED (elan_cmd_precapture_del); ALLOWED (elan_cmd_height); ALLOWED (elan_cmd_width);
  ALLOWED (elan_cmd_fw_version); ALLOWED (elan_cmd_sample_version); ALLOWED (elan_cmd_sso_on);
  ALLOWED (elan_cmd_recognize); ALLOWED (elan_cmd_session_status); ALLOWED (elan_cmd_calib_mean);
  ALLOWED (elan_cmd_calib_start); ALLOWED (elan_cmd_wait_release); ALLOWED (elan_cmd_wait_press);
  ALLOWED (elan_cmd_gpio_low); ALLOWED (elan_cmd_image);
}

static void
test_release_score (void)
{
  g_assert_cmpint (elan_release_score (0x2410), ==, 100);
  g_assert_cmpint (elan_release_score (0x2411), ==, 0);
  g_assert_cmpint (elan_release_score (0x2400), ==, 0);
  g_assert_cmpint (elan_release_score (0), ==, 0);
}

static void
test_scan_codes (void)
{
  const guint8 press[] = { 0xAF, 0x55 }, release[] = { 0xAA };
  const guint8 a[] = { 0xAF }, ff[] = { 0xFF, 0xFF }, late[] = { 0xFF, 0x55 };
  const guint8 lost[] = { 0xCC, 0xAF }, first[] = { 0xAF, 0xCC }, unk[] = { 0x9A, 0x10, 0x00 };
  guint8 found = 0;

  g_assert_cmpint (elan_scan_codes (a, 1, press, 2, &found), ==, ELAN_SCAN_FOUND);
  g_assert_cmphex (found, ==, 0xAF);
  g_assert_cmpint (elan_scan_codes (late, 2, press, 2, &found), ==, ELAN_SCAN_FOUND);
  g_assert_cmphex (found, ==, 0x55);
  g_assert_cmpint (elan_scan_codes (ff, 2, press, 2, &found), ==, ELAN_SCAN_NONE);
  g_assert_cmpint (elan_scan_codes (unk, 3, press, 2, &found), ==, ELAN_SCAN_NONE);
  g_assert_cmpint (elan_scan_codes (a, 1, release, 1, &found), ==, ELAN_SCAN_NONE);
  g_assert_cmpint (elan_scan_codes (lost, 2, press, 2, &found), ==, ELAN_SCAN_KEY_LOST);
  g_assert_cmpint (elan_scan_codes (first, 2, press, 2, &found), ==, ELAN_SCAN_FOUND);
  g_assert_cmpint (elan_scan_codes (NULL, 0, press, 2, &found), ==, ELAN_SCAN_NONE);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);
  g_test_add_func ("/guard/forbidden-commands", test_forbidden_commands);
  g_test_add_func ("/guard/driver-commands-allowed", test_driver_commands_are_allowed);
  g_test_add_func ("/guard/release-score", test_release_score);
  g_test_add_func ("/guard/scan-codes", test_scan_codes);
  return g_test_run ();
}
