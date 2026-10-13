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

/*
 * Builds the template stored in the print from the n_frames touches the
 * enroll accepted (ELAN_ENROLL_STAGES, each a different contact of the same
 * finger). The print also keeps their 8-bit images (elan0c4d-print.c).
 */
gboolean elan_match_enroll (const ElanFrame *frames, guint n_frames, GBytes **tmpl, GError **error);

/* Compares a usable finger frame with a stored template; *matched only if TRUE is returned. */
gboolean elan_match_verify (const ElanFrame *frame, GBytes *tmpl, gboolean *matched, GError **error);
