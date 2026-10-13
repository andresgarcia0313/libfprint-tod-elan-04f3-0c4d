/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* Enrolled synthetic surfaces shared by test-match.c and test-match-data.c. */
#include "fpi-device.h"
#include "elan0c4d-match.h"
#include "synth.h"
#include "test-match.h"

guint16 shots[4][ELAN_PIXELS];

/* Template from four windows of surface `seed`, shifted like four different contacts. */
GBytes *
enroll_surface (guint32 seed)
{
  static const guint offsets[4][2] = { { 0, 0 }, { 20, 10 }, { 40, 25 }, { 10, 45 } };
  ElanFrame frames[4];
  GBytes *tmpl = NULL;
  g_autoptr(GError) error = NULL;

  for (guint i = 0; i < 4; i++)
    {
      synth_texture (shots[i], seed, offsets[i][0], offsets[i][1]);
      frames[i] = (ElanFrame) { shots[i], shots[i], ELAN_WIDTH, ELAN_HEIGHT };
    }
  g_assert_true (elan_match_enroll (frames, 4, &tmpl, &error));
  g_assert_no_error (error);
  return tmpl;
}
