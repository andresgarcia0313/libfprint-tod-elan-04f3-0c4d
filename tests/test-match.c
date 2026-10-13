/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* The comparator on synthetic surfaces: the same surface is accepted, any other refused. */
#include "fpi-device.h"
#include "elan0c4d-match.h"
#include "synth.h"
#include "test-match.h"

static gboolean
verify_surface (GBytes *tmpl, guint32 seed, guint dx, guint dy)
{
  static guint16 px[ELAN_PIXELS];
  ElanFrame frame = { px, px, ELAN_WIDTH, ELAN_HEIGHT };
  gboolean matched = FALSE;
  g_autoptr(GError) error = NULL;

  synth_texture (px, seed, dx, dy);
  g_assert_true (elan_match_verify (&frame, tmpl, &matched, &error));
  g_assert_no_error (error);
  return matched;
}

static void
test_same_surface_matches (void)
{
  g_autoptr(GBytes) tmpl = enroll_surface (7);

  g_assert_true (verify_surface (tmpl, 7, 5, 3));    /* near the first contact */
  g_assert_true (verify_surface (tmpl, 7, 30, 18));  /* between two contacts */
}

static void
test_other_surface_is_refused (void)
{
  g_autoptr(GBytes) tmpl = enroll_surface (7);

  for (guint32 seed = 100; seed < 110; seed++)
    g_assert_false (verify_surface (tmpl, seed, 5, 3));
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);
  g_test_add_func ("/match/same-surface-matches", test_same_surface_matches);
  g_test_add_func ("/match/other-surface-is-refused", test_other_surface_is_refused);
  return g_test_run ();
}
