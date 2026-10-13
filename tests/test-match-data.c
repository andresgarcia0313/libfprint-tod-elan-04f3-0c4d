/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* The comparator never trusts stored bytes and fails cleanly on images without features. */
#include <string.h>
#include "fpi-device.h"
#include "elan0c4d-match.h"
#include "synth.h"
#include "test-match.h"

static void
check_bad_template (const void *data, gsize size)
{
  static guint16 px[ELAN_PIXELS];
  ElanFrame frame = { px, px, ELAN_WIDTH, ELAN_HEIGHT };
  g_autoptr(GBytes) tmpl = g_bytes_new (data, size);
  g_autoptr(GError) error = NULL;
  gboolean matched = TRUE;

  synth_texture (px, 7, 0, 0);
  g_assert_false (elan_match_verify (&frame, tmpl, &matched, &error));
  g_assert_error (error, FP_DEVICE_ERROR, FP_DEVICE_ERROR_DATA_INVALID);
  g_assert_nonnull (strstr (error->message, "template"));  /* the reason reaches the log */
  g_assert_true (matched);  /* untouched on error */
}

static void
test_bad_templates_are_data_errors (void)
{
  g_autoptr(GBytes) good = enroll_surface (7);
  gsize size = 0;
  const guint8 *bytes = g_bytes_get_data (good, &size);
  g_autofree guint8 *copy = g_memdup2 (bytes, size + 1);

  check_bad_template ("", 0);
  check_bad_template ("XXXX\1\0", 6);              /* foreign magic */
  check_bad_template ("E0S1\0\0", 6);              /* no views */
  check_bad_template ("E0S1\1\0\xff\xff", 8);      /* more points than allowed */
  check_bad_template (bytes, size - 1);            /* truncated */
  check_bad_template (copy, size + 1);             /* trailing byte */
}

static void
test_featureless_enroll_fails (void)
{
  ElanFrame frame = { shots[0], shots[0], ELAN_WIDTH, ELAN_HEIGHT };
  GBytes *tmpl = NULL;
  g_autoptr(GError) error = NULL;

  synth_flat (shots[0], 3050);
  g_assert_false (elan_match_enroll (&frame, 1, &tmpl, &error));
  g_assert_error (error, FP_DEVICE_ERROR, FP_DEVICE_ERROR_GENERAL);
  g_assert_null (tmpl);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);
  g_test_add_func ("/match/bad-templates-are-data-errors", test_bad_templates_are_data_errors);
  g_test_add_func ("/match/featureless-enroll-fails", test_featureless_enroll_fails);
  return g_test_run ();
}
