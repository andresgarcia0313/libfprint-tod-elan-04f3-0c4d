// SPDX-License-Identifier: LGPL-2.1-or-later
// The C entry points of the comparator: every exception stops here and becomes
// a status plus a message, so no C++ or OpenCV error ever reaches libfprint.
#include <new>
#include <opencv2/core.hpp>
#include "elan0c4d-sift.h"
#include "elan0c4d-sift.hpp"

namespace
{
// Runs `body`, turning each kind of failure into its status and a message.
template <typename Body>
ElanSiftStatus
guarded (const char *what, gchar **message, Body body)
{
  try
    {
      return body ();
    }
  catch (const elan::TemplateError &e)
    {
      *message = g_strdup_printf ("%s: %s", what, e.what ());
      return ELAN_SIFT_BAD_TEMPLATE;
    }
  catch (const cv::Exception &e)
    {
      *message = g_strdup_printf ("%s: OpenCV error %d in %s: %s", what, e.code, e.func.c_str (), e.err.c_str ());
    }
  catch (const std::bad_alloc &)
    {
      *message = g_strdup_printf ("%s: out of memory", what);
    }
  catch (const std::exception &e)
    {
      *message = g_strdup_printf ("%s: %s", what, e.what ());
    }
  catch (...)
    {
      *message = g_strdup_printf ("%s: unknown C++ exception", what);
    }
  return ELAN_SIFT_FAILED;
}
}  // namespace

extern "C" ElanSiftStatus
elan_sift_enroll (const guint8 *const *images, guint n, guint width, guint height, GBytes **tmpl, gchar **message)
{
  return guarded ("enroll template", message, [&] {
    std::vector<elan::View> views;
    gsize points = 0;
    for (guint i = 0; i < n; i++)
      {
        views.push_back (elan::features (images[i], width, height));
        points += views.back ().points.size ();
        g_debug ("enroll view %u: %zu keypoints", i + 1, views.back ().points.size ());
      }
    if (points == 0)
      {
        *message = g_strdup ("enroll template: no keypoints in any touch");
        return ELAN_SIFT_NO_FEATURES;
      }
    *tmpl = elan::pack (views);
    return ELAN_SIFT_OK;
  });
}

extern "C" ElanSiftStatus
elan_sift_score (const guint8 *image, guint width, guint height, GBytes *tmpl, gint *best, gchar **message)
{
  return guarded ("compare", message, [&] {
    std::vector<elan::View> views = elan::unpack (tmpl);
    elan::View probe = elan::features (image, width, height);
    *best = 0;
    for (gsize i = 0; i < views.size (); i++)
      {
        int score = elan::consistent_matches (probe, views[i]);
        g_debug ("compare: view %zu gives %d consistent matches", i + 1, score);
        *best = MAX (*best, score);
      }
    g_debug ("compare: probe has %zu keypoints, best view %d", probe.points.size (), *best);
    return ELAN_SIFT_OK;
  });
}
