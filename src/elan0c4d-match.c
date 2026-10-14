/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * The comparator the driver calls: 8-bit images (elan_quality_normalize) go
 * to the OpenCV matcher and its statuses become libfprint errors.
 * Each result is a g_message, so it reaches the journal without debug: one
 * line per touch with its score, never the image.
 */
#include "fpi-device.h"
#include "elan0c4d-match.h"
#include "elan0c4d-quality.h"
#include "elan0c4d-sift.h"

static gboolean
check (ElanSiftStatus status, gchar *message, GError **error)
{
  g_autofree gchar *owned = message;

  switch (status)
    {
    case ELAN_SIFT_OK:
      return TRUE;
    case ELAN_SIFT_BAD_TEMPLATE:
      g_set_error (error, FP_DEVICE_ERROR, FP_DEVICE_ERROR_DATA_INVALID, "%s", owned);
      return FALSE;
    case ELAN_SIFT_NO_FEATURES:
    case ELAN_SIFT_FAILED:
    default:
      g_set_error (error, FP_DEVICE_ERROR, FP_DEVICE_ERROR_GENERAL, "%s",
                   owned ? owned : "comparator failed without a message");
      return FALSE;
    }
}

gboolean
elan_match_enroll (const ElanFrame *frames, guint n_frames, GBytes **tmpl, GError **error)
{
  g_autoptr(GPtrArray) images = g_ptr_array_new_with_free_func (g_free);
  ElanSiftStatus status;
  gchar *message = NULL;

  for (guint i = 0; i < n_frames; i++)
    {
      guint8 *img = g_malloc ((gsize) frames[i].width * frames[i].height);

      elan_quality_normalize (frames[i].pixels, frames[i].width, frames[i].height, img);
      g_ptr_array_add (images, img);
    }
  if (n_frames == 0)
    return check (ELAN_SIFT_NO_FEATURES, g_strdup ("enroll template: no touches"), error);
  /* Two statements: `message` is only set once the call has returned. */
  status = elan_sift_enroll ((const guint8 *const *) images->pdata, n_frames, frames[0].width,
                             frames[0].height, tmpl, &message);
  if (!check (status, message, error))
    return FALSE;
  g_message ("enroll: template of %u touches, %zu bytes", n_frames, g_bytes_get_size (*tmpl));
  return TRUE;
}

gboolean
elan_match_verify (const ElanFrame *frame, GBytes *tmpl, gboolean *matched, GError **error)
{
  g_autofree guint8 *img = g_malloc ((gsize) frame->width * frame->height);
  ElanSiftStatus status;
  gchar *message = NULL;
  gint best = 0;

  elan_quality_normalize (frame->pixels, frame->width, frame->height, img);
  status = elan_sift_score (img, frame->width, frame->height, tmpl, &best, &message);
  if (!check (status, message, error))
    return FALSE;
  *matched = best >= ELAN_MATCH_MIN_INLIERS;
  g_message ("verify: %d consistent matches (need %d): %s", best, ELAN_MATCH_MIN_INLIERS,
          *matched ? "match" : "no match");
  return TRUE;
}
