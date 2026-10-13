/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* Classifier tests on SYNTHETIC frames (no biometric data): empty, ridges, zeros, saturation. */
#include <glib.h>
#include "elan0c4d-proto.h"
#include "elan0c4d-quality.h"
#include "synth.h"

static ElanAssessment
assess (const guint16 *px)
{
  ElanAssessment a = elan_quality_assess (px, ELAN_WIDTH, ELAN_HEIGHT);

  g_test_message ("mean=%.0f kind=%s coverage=%.2f ridge=%.0f", a.mean, elan_frame_kind_name (a.kind),
                  a.coverage, a.ridge);
  return a;
}

static void
test_empty (void)
{
  guint16 px[ELAN_PIXELS];
  ElanAssessment a;

  synth_empty (px);
  a = assess (px);
  g_assert_cmpint (a.kind, ==, ELAN_FRAME_EMPTY);
  g_assert_cmpfloat (a.coverage, ==, 0.0);
  g_assert_cmpfloat (a.ridge, <, 112.0);
}

static void
test_finger (void)
{
  guint16 px[ELAN_PIXELS];
  ElanAssessment a;

  synth_ridges (px, ELAN_HEIGHT / 10);
  a = assess (px);
  g_assert_cmpint (a.kind, ==, ELAN_FRAME_FINGER);
  g_assert_cmpfloat (a.coverage, ==, 1.0);
  g_assert_cmpfloat (a.ridge, >=, 490.0);
}

/* All zeros and the saturated 0x3FFE are invalid; 200 and 15000 are the first usable means. */
static void
test_mean_limits (void)
{
  static const struct { guint16 value; ElanFrameKind kind; } cases[] = {
    { 0, ELAN_FRAME_INVALID }, { 0x3FFE, ELAN_FRAME_INVALID }, { 199, ELAN_FRAME_INVALID },
    { 200, ELAN_FRAME_EMPTY }, { 15000, ELAN_FRAME_EMPTY }, { 15001, ELAN_FRAME_INVALID },
  };
  guint16 px[ELAN_PIXELS];

  for (guint i = 0; i < G_N_ELEMENTS (cases); i++)
    {
      synth_flat (px, cases[i].value);
      g_assert_cmpint (assess (px).kind, ==, cases[i].kind);
    }
}

/* 15 x 5 blocks: 60 % of the blocks is the first coverage that counts as a print. */
static void
test_coverage_threshold (void)
{
  guint16 px[ELAN_PIXELS];

  synth_ridges (px, 8);
  g_assert_cmpint (assess (px).kind, ==, ELAN_FRAME_EMPTY);
  synth_ridges (px, 9);
  g_assert_cmpint (assess (px).kind, ==, ELAN_FRAME_FINGER);
  g_assert_cmpfloat_with_epsilon (assess (px).coverage, 0.6, 1e-9);
}

static void
test_degenerate_geometry (void)
{
  guint16 px[9 * 9];

  for (guint i = 0; i < G_N_ELEMENTS (px); i++)
    px[i] = 3000;
  g_assert_cmpint (elan_quality_assess (px, 9, 9).kind, ==, ELAN_FRAME_EMPTY);
  g_assert_cmpint (elan_quality_assess (px, 0, 0).kind, ==, ELAN_FRAME_INVALID);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);
  g_test_add_func ("/quality/empty", test_empty);
  g_test_add_func ("/quality/finger", test_finger);
  g_test_add_func ("/quality/zeros-saturation-and-mean-limits", test_mean_limits);
  g_test_add_func ("/quality/coverage-threshold", test_coverage_threshold);
  g_test_add_func ("/quality/degenerate-geometry", test_degenerate_geometry);
  return g_test_run ();
}
