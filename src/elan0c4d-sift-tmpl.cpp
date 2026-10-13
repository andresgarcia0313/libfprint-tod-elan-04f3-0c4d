// SPDX-License-Identifier: LGPL-2.1-or-later
// Template bytes, little endian: "E0S1", u16 views, then per view u16 points,
// points x (f32 x, f32 y) and points x 128 descriptor bytes. Only keypoint
// positions and descriptors are kept: no image of the finger is stored.
#include "elan0c4d-sift-bytes.hpp"

namespace
{
constexpr char kMagic[4] = { 'E', '0', 'S', '1' };
constexpr int kDescriptorBytes = 128;
constexpr guint kMaxViews = 64;
}  // namespace

namespace elan
{

GBytes *
pack (const std::vector<View> &views)
{
  if (views.empty () || views.size () > kMaxViews)
    throw std::length_error ("template needs 1 to 64 views");
  g_autoptr(GByteArray) out = g_byte_array_new ();
  g_byte_array_append (out, reinterpret_cast<const guint8 *> (kMagic), sizeof kMagic);
  put_u16 (out, views.size ());
  for (const View &v : views)
    {
      gsize n = v.points.size ();
      if (n > 0 && (v.descriptors.rows != static_cast<int> (n) || v.descriptors.cols != kDescriptorBytes ||
                    v.descriptors.type () != CV_8U || !v.descriptors.isContinuous ()))
        throw std::logic_error ("view descriptors do not match its points");
      put_u16 (out, n);
      for (const cv::Point2f &p : v.points)
        {
          put_f32 (out, p.x);
          put_f32 (out, p.y);
        }
      if (n > 0)
        g_byte_array_append (out, v.descriptors.ptr<guint8> (), n * kDescriptorBytes);
    }
  return g_byte_array_free_to_bytes (static_cast<GByteArray *> (g_steal_pointer (&out)));
}

std::vector<View>
unpack (GBytes *tmpl)
{
  gsize size = 0;
  Reader r { static_cast<const guint8 *> (g_bytes_get_data (tmpl, &size)), size };
  if (std::memcmp (r.take (sizeof kMagic), kMagic, sizeof kMagic) != 0)
    throw TemplateError ("not an elan0c4d SIFT template");
  guint n_views = r.u16 ();
  if (n_views == 0 || n_views > kMaxViews)
    throw TemplateError ("template has an impossible number of views");
  std::vector<View> views (n_views);
  for (View &v : views)
    {
      guint n = r.u16 ();
      if (n > kMaxCount)
        throw TemplateError ("template view has too many points");
      for (guint i = 0; i < n; i++)
        {
          float x = r.coordinate ();  // two statements: argument order is unspecified
          v.points.emplace_back (x, r.coordinate ());
        }
      const guint8 *bytes = r.take (static_cast<gsize> (n) * kDescriptorBytes);
      v.descriptors = cv::Mat (static_cast<int> (n), kDescriptorBytes, CV_8U, const_cast<guint8 *> (bytes)).clone ();
    }
  if (r.at != size)
    throw TemplateError ("template has trailing bytes");
  return views;
}

}  // namespace elan
