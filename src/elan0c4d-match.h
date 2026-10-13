/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Interface of the fingerprint comparator. The matching algorithm is NOT
 * decided yet: the driver only knows this header. Replace
 * elan0c4d-match-stub.c in meson.build with a real implementation; the
 * stub fails every call with FP_DEVICE_ERROR_NOT_SUPPORTED.
 */
#pragma once

#include <glib.h>

typedef struct
{
  const guint16 *pixels;      /* width x height raw 14-bit samples, row after row */
  const guint16 *background;  /* background taken before the capture, same geometry */
  guint          width;
  guint          height;
} ElanFrame;

/* Builds the template stored in the print from one usable finger frame. */
gboolean elan_match_enroll (const ElanFrame *frame, GBytes **tmpl, GError **error);

/* Compares a usable finger frame with a stored template; *matched only if TRUE is returned. */
gboolean elan_match_verify (const ElanFrame *frame, GBytes *tmpl, gboolean *matched, GError **error);
