/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * 8-bit image of a frame (elan_quality.py normalized) and how much two of
 * them look alike. Pure arithmetic: shared by the driver and the tests.
 */
#include <math.h>
#include "elan0c4d-quality.h"

static int
compare_double (gconstpointer a, gconstpointer b)
{
  gdouble x = *(const gdouble *) a, y = *(const gdouble *) b;

  return (x > y) - (x < y);
}

/* numpy.percentile with its default linear method (and its lerp), on sorted values. */
static gdouble
percentile (const gdouble *sorted, gsize n, gdouble q)
{
  gdouble pos = q / 100.0 * (gdouble) (n - 1);
  gsize i = (gsize) pos;
  gdouble t = pos - (gdouble) i, a = sorted[i], b = sorted[MIN (i + 1, n - 1)];

  return t >= 0.5 ? b - (b - a) * (1.0 - t) : a + (b - a) * t;
}

void
elan_quality_normalize (const guint16 *px, guint width, guint height, guint8 *out)
{
  gsize n = (gsize) width * height;
  g_autofree gdouble *centered = g_new (gdouble, n);
  g_autofree gdouble *sorted = NULL;
  gdouble lo, hi;

  if (n == 0)
    return;
  for (guint x = 0; x < width; x++)  /* the per-column fixed pattern goes away */
    {
      gdouble sum = 0.0;

      for (guint y = 0; y < height; y++)
        sum += px[(gsize) y * width + x];
      for (guint y = 0; y < height; y++)
        centered[(gsize) y * width + x] = px[(gsize) y * width + x] - sum / height;
    }
  sorted = g_memdup2 (centered, n * sizeof *centered);
  qsort (sorted, n, sizeof *sorted, compare_double);
  lo = percentile (sorted, n, 1.0);
  hi = percentile (sorted, n, 99.0);
  for (gsize i = 0; i < n; i++)
    out[i] = (guint8) CLAMP ((centered[i] - lo) * 255.0 / MAX (hi - lo, 1.0), 0.0, 255.0);
}

gdouble
elan_quality_similarity (const guint8 *a, const guint8 *b, gsize n)
{
  gdouble mean_a = 0.0, mean_b = 0.0, ab = 0.0, aa = 0.0, bb = 0.0;

  if (n == 0)
    return 0.0;
  for (gsize i = 0; i < n; i++)
    {
      mean_a += a[i];
      mean_b += b[i];
    }
  mean_a /= n;
  mean_b /= n;
  for (gsize i = 0; i < n; i++)
    {
      gdouble da = a[i] - mean_a, db = b[i] - mean_b;

      ab += da * db;
      aa += da * da;
      bb += db * db;
    }
  return aa > 0.0 && bb > 0.0 ? ab / sqrt (aa * bb) : 0.0;
}
