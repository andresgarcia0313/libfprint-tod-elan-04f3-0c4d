/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Smooth random texture for the comparator tests: two octaves of value noise
 * on a surface larger than the sensor. Not a fingerprint, but it has corners
 * and blobs SIFT can hold on to, and the same seed gives the same surface.
 */
#include "synth.h"

#define SURFACE_W 96
#define SURFACE_H 200

static gdouble
lattice (guint32 seed, gint cx, gint cy)
{
  guint32 h = seed * 374761393u + (guint32) cx * 668265263u + (guint32) cy * 2246822519u;

  h = (h ^ (h >> 13)) * 1274126177u;
  return (gdouble) ((h ^ (h >> 16)) & 0xFFFF) / 65535.0;
}

static gdouble
noise (guint32 seed, gdouble x, gdouble y, gdouble cell)
{
  gint cx = (gint) (x / cell), cy = (gint) (y / cell);
  gdouble fx = x / cell - cx, fy = y / cell - cy;
  gdouble top = lattice (seed, cx, cy) * (1 - fx) + lattice (seed, cx + 1, cy) * fx;
  gdouble bottom = lattice (seed, cx, cy + 1) * (1 - fx) + lattice (seed, cx + 1, cy + 1) * fx;

  return top * (1 - fy) + bottom * fy;
}

void
synth_texture (guint16 *px, guint32 seed, guint dx, guint dy)
{
  g_assert_cmpuint (dx + ELAN_WIDTH, <=, SURFACE_W);
  g_assert_cmpuint (dy + ELAN_HEIGHT, <=, SURFACE_H);
  for (guint y = 0; y < ELAN_HEIGHT; y++)
    for (guint x = 0; x < ELAN_WIDTH; x++)
      {
        gdouble sx = x + dx, sy = y + dy;
        gdouble v = 0.65 * noise (seed, sx, sy, 7.0) + 0.35 * noise (seed + 1, sx, sy, 3.0);

        px[y * ELAN_WIDTH + x] = (guint16) (3000 + 6000 * v);
      }
}
