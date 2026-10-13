/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* 8-bit image of a frame (elan_quality.py normalized) and similarity of two touches. */
#include "elan0c4d-quality.h"
#include "synth.h"

/* One column 0..100: centered -50..50, percentiles -49 and 49, so known 8-bit values. */
static void
test_stretch_between_percentiles (void)
{
  guint16 px[101];
  guint8 out[101];

  for (guint i = 0; i < 101; i++)
    px[i] = i;
  elan_quality_normalize (px, 1, 101, out);
  g_assert_cmpuint (out[0], ==, 0);
  g_assert_cmpuint (out[1], ==, 0);
  g_assert_cmpuint (out[25], ==, 62);   /* (-25 + 49) * 255 / 98 = 62.4 */
  g_assert_cmpuint (out[50], ==, 127);  /* 127.5 is truncated, as astype(uint8) does */
  g_assert_cmpuint (out[99], ==, 255);
  g_assert_cmpuint (out[100], ==, 255);
}

/* The per-column fixed pattern goes away: a column 1000 brighter gives the same image. */
static void
test_column_pattern_removed (void)
{
  guint16 px[2 * 60];
  guint8 out[2 * 60];

  for (guint y = 0; y < 60; y++)
    {
      px[2 * y] = 3000 + (y * 37) % 200;
      px[2 * y + 1] = px[2 * y] + 1000;
    }
  elan_quality_normalize (px, 2, 60, out);
  for (guint y = 0; y < 60; y++)
    g_assert_cmpuint (out[2 * y], ==, out[2 * y + 1]);
}

static void
test_flat_frame_is_black (void)
{
  guint16 px[ELAN_PIXELS];
  guint8 out[ELAN_PIXELS];

  synth_flat (px, 3050);
  elan_quality_normalize (px, ELAN_WIDTH, ELAN_HEIGHT, out);
  for (guint i = 0; i < ELAN_PIXELS; i++)
    g_assert_cmpuint (out[i], ==, 0);
}

static void
test_similarity (void)
{
  guint16 px[ELAN_PIXELS];
  guint8 a[ELAN_PIXELS], b[ELAN_PIXELS], inverse[ELAN_PIXELS], flat[ELAN_PIXELS] = { 0 };

  synth_ridges (px, ELAN_HEIGHT / 10);
  elan_quality_normalize (px, ELAN_WIDTH, ELAN_HEIGHT, a);
  synth_ridges_at (px, ELAN_HEIGHT / 10, 1);
  elan_quality_normalize (px, ELAN_WIDTH, ELAN_HEIGHT, b);
  for (guint i = 0; i < ELAN_PIXELS; i++)
    inverse[i] = 255 - a[i];
  g_assert_cmpfloat_with_epsilon (elan_quality_similarity (a, a, ELAN_PIXELS), 1.0, 1e-9);
  g_assert_cmpfloat_with_epsilon (elan_quality_similarity (a, inverse, ELAN_PIXELS), -1.0, 1e-9);
  g_assert_cmpfloat (elan_quality_similarity (a, b, ELAN_PIXELS), <, 0.5);
  g_assert_cmpfloat (elan_quality_similarity (a, flat, ELAN_PIXELS), ==, 0.0);
  g_assert_cmpfloat (elan_quality_similarity (a, b, 0), ==, 0.0);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);
  g_test_add_func ("/normalize/stretch-between-percentiles", test_stretch_between_percentiles);
  g_test_add_func ("/normalize/column-pattern-removed", test_column_pattern_removed);
  g_test_add_func ("/normalize/flat-frame-is-black", test_flat_frame_is_black);
  g_test_add_func ("/normalize/similarity", test_similarity);
  return g_test_run ();
}
