/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Local check of the comparator on real touches saved by huella-proto
 * (finger<N>.npy, int32 150x52). Enrolls the first 12 of <mine>, then scores
 * the rest of <mine> and every touch of <other>. Fingerprint captures are
 * personal data: they stay on the machine and never enter the repository.
 * Usage: match-eval <mine_dir> <mine_count> <other_dir> <other_count>
 */
#include <stdlib.h>
#include "elan0c4d-match.h"
#include "elan0c4d-proto.h"
#include "elan0c4d-quality.h"
#include "elan0c4d-sift.h"

#define ENROLLED 12

/* finger<n>.npy -> 8-bit image; the npy header ends with '\n' before the data. */
static guint8 *
load (const char *dir, guint n)
{
  g_autofree gchar *path = g_strdup_printf ("%s/finger%u.npy", dir, n);
  g_autofree gchar *data = NULL;
  g_autoptr(GError) error = NULL;
  guint16 px[ELAN_PIXELS];
  gsize size = 0, header;
  guint8 *out = g_malloc (ELAN_PIXELS);

  if (!g_file_get_contents (path, &data, &size, &error))
    g_error ("%s", error->message);
  header = 10 + (guint8) data[8] + 256u * (guint8) data[9];
  if (size != header + ELAN_PIXELS * 4)
    g_error ("%s: expected int32 %dx%d", path, ELAN_HEIGHT, ELAN_WIDTH);
  for (guint i = 0; i < ELAN_PIXELS; i++)
    px[i] = (guint16) GINT32_FROM_LE (((const gint32 *) (data + header))[i]);
  elan_quality_normalize (px, ELAN_WIDTH, ELAN_HEIGHT, out);
  return out;
}

static gint
score (const char *dir, guint n, GBytes *tmpl)
{
  g_autofree guint8 *img = load (dir, n);
  g_autofree gchar *message = NULL;
  gint best = -1;

  if (elan_sift_score (img, ELAN_WIDTH, ELAN_HEIGHT, tmpl, &best, &message) != ELAN_SIFT_OK)
    g_error ("%s", message);
  return best;
}

int
main (int argc, char **argv)
{
  guint8 *views[ENROLLED];
  g_autofree gchar *message = NULL;
  g_autoptr(GBytes) tmpl = NULL;
  guint mine, other, accepted = 0, impostors = 0;

  if (argc != 5)
    g_error ("usage: %s <mine_dir> <mine_count> <other_dir> <other_count>", argv[0]);
  mine = atoi (argv[2]);
  other = atoi (argv[4]);
  for (guint i = 0; i < ENROLLED; i++)
    views[i] = load (argv[1], i + 1);
  if (elan_sift_enroll ((const guint8 *const *) views, ENROLLED, ELAN_WIDTH, ELAN_HEIGHT, &tmpl, &message) != ELAN_SIFT_OK)
    g_error ("%s", message);
  g_print ("template: %u touches, %zu bytes\n", ENROLLED, g_bytes_get_size (tmpl));
  for (guint n = ENROLLED + 1; n <= mine; n++)
    {
      gint s = score (argv[1], n, tmpl);
      accepted += s >= ELAN_MATCH_MIN_INLIERS;
      g_print ("mine  %2u: %3d\n", n, s);
    }
  for (guint n = 1; n <= other; n++)
    {
      gint s = score (argv[3], n, tmpl);
      impostors += s >= ELAN_MATCH_MIN_INLIERS;
      g_print ("other %2u: %3d\n", n, s);
    }
  g_print ("accepted %u/%u of mine, %u/%u of the other finger (need %d)\n", accepted, mine - ENROLLED,
           impostors, other, ELAN_MATCH_MIN_INLIERS);
  for (guint i = 0; i < ENROLLED; i++)
    g_free (views[i]);
  return 0;
}
