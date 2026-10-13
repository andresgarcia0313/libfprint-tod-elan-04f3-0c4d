/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "synth.h"

static guint32 seed = 12345;

static guint32
next_rand (void)
{
  seed = seed * 1664525u + 1013904223u;
  return seed >> 8;
}

void
synth_flat (guint16 *px, guint16 value)
{
  for (guint i = 0; i < ELAN_PIXELS; i++)
    px[i] = value;
}

void
synth_empty (guint16 *px)
{
  for (guint i = 0; i < ELAN_PIXELS; i++)
    px[i] = 3050 + (next_rand () % 81) - 40;
}

void
synth_ridges (guint16 *px, guint block_rows)
{
  static const guint16 crest[4] = { 0, 600, 1200, 600 };

  synth_empty (px);
  for (guint y = 0; y < block_rows * 10; y++)
    for (guint x = 0; x < ELAN_WIDTH; x++)
      px[y * ELAN_WIDTH + x] = 2450 + crest[(x + y / 2) % 4];
}
