// SPDX-License-Identifier: LGPL-2.1-or-later
// Little-endian writer and bounded reader for the template (elan0c4d-sift-tmpl.cpp).
#pragma once

#include <cmath>
#include <cstring>
#include "elan0c4d-sift.hpp"

namespace elan
{

constexpr gsize kMaxCount = 4096;  // views per template and points per view

inline void
put_u16 (GByteArray *out, gsize v)
{
  if (v > kMaxCount)
    throw std::length_error ("too many views or points for a template");
  guint16 le = GUINT16_TO_LE (static_cast<guint16> (v));
  g_byte_array_append (out, reinterpret_cast<const guint8 *> (&le), sizeof le);
}

inline void
put_f32 (GByteArray *out, float v)
{
  guint32 bits;
  std::memcpy (&bits, &v, sizeof bits);
  bits = GUINT32_TO_LE (bits);
  g_byte_array_append (out, reinterpret_cast<const guint8 *> (&bits), sizeof bits);
}

// Every take() checks that the bytes are there before handing them out.
struct Reader
{
  const guint8 *data;
  gsize size, at = 0;

  const guint8 *take (gsize n)
  {
    if (n > size - at)
      throw TemplateError ("template truncated");
    return data + (at += n) - n;
  }

  guint u16 ()
  {
    guint16 v;
    std::memcpy (&v, take (2), 2);
    return GUINT16_FROM_LE (v);
  }

  float coordinate ()
  {
    guint32 bits;
    float v;
    std::memcpy (&bits, take (4), 4);
    bits = GUINT32_FROM_LE (bits);
    std::memcpy (&v, &bits, 4);
    if (!std::isfinite (v))
      throw TemplateError ("template point is not a number");
    return v;
  }
};

}  // namespace elan
