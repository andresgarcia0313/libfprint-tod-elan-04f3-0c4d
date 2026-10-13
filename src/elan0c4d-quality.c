/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Measured on real captures: mean |gradient| per 10x10 block is 41-112 on an
 * empty sensor and 575-742 with a finger. All-zero and saturated (0x3FFE)
 * frames mean the analog front end lost calibration and are INVALID. The
 * background is never used to decide (a finger resting while it was taken
 * would cancel itself out).
 */
#include "elan0c4d-quality.h"

#define BLOCK 10
#define RIDGE_MIN 300           /* block energy that counts as skin */
#define COVERAGE_MIN 0.6        /* share of skin blocks for a usable print */
#define MEAN_MIN 200.0          /* below: zeros */
#define MEAN_MAX 15000.0        /* above: saturation */

/* |gradient| of one pixel: vertical plus horizontal step, as in the reference. */
static guint
pixel_gradient (const guint16 *px, guint width, guint x, guint y)
{
  const guint16 *p = px + (gsize) y * width + x;
  guint g = 0;

  if (y > 0)
    g += (guint) ABS ((gint) p[0] - (gint) p[-(gssize) width]);
  if (x > 0)
    g += (guint) ABS ((gint) p[0] - (gint) p[-1]);
  return g;
}

static guint64
block_energy (const guint16 *px, guint width, guint bx, guint by)
{
  guint64 sum = 0;

  for (guint y = by * BLOCK; y < (by + 1) * BLOCK; y++)
    for (guint x = bx * BLOCK; x < (bx + 1) * BLOCK; x++)
      sum += pixel_gradient (px, width, x, y);
  return sum;
}

ElanAssessment
elan_quality_assess (const guint16 *px, guint width, guint height)
{
  ElanAssessment a = { ELAN_FRAME_INVALID, 0.0, 0.0, 0.0 };
  guint blocks_x = width / BLOCK, blocks_y = height / BLOCK, skin = 0;
  gsize n = (gsize) width * height;
  gdouble total = 0.0, energy = 0.0;

  for (gsize i = 0; i < n; i++)
    total += px[i];
  a.mean = n ? total / n : 0.0;
  if (a.mean < MEAN_MIN || a.mean > MEAN_MAX)
    return a;

  a.kind = ELAN_FRAME_EMPTY;
  if (blocks_x == 0 || blocks_y == 0)
    return a;

  for (guint by = 0; by < blocks_y; by++)
    for (guint bx = 0; bx < blocks_x; bx++)
      {
        guint64 sum = block_energy (px, width, bx, by);

        energy += (gdouble) sum / (BLOCK * BLOCK);
        if (sum >= (guint64) RIDGE_MIN * BLOCK * BLOCK)
          skin++;
      }
  a.coverage = (gdouble) skin / (blocks_x * blocks_y);
  a.ridge = energy / (blocks_x * blocks_y);
  if (a.coverage >= COVERAGE_MIN)
    a.kind = ELAN_FRAME_FINGER;
  return a;
}

const char *
elan_frame_kind_name (ElanFrameKind kind)
{
  switch (kind)
    {
    case ELAN_FRAME_FINGER:
      return "finger";
    case ELAN_FRAME_INVALID:
      return "invalid";
    default:
      return "empty";
    }
}
