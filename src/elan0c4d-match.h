/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Interface of the fingerprint comparator (elan0c4d-match.c, built on the
 * OpenCV keypoint matcher of elan0c4d-sift*.cpp). The driver only knows this
 * header; elan0c4d-match-stub.c is the same interface without a matcher.
 */
#pragma once

#include <glib.h>

/*
 * A probe matches when this many keypoint matches agree on one placement
 * with some enrolled touch. Measured on the real sensor (2026-10-13):
 * the other finger never went above 3, overlapping genuine touches gave 11+.
 */
#define ELAN_MATCH_MIN_INLIERS 8

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
 * finger). The template holds keypoints and descriptors, never the images.
 */
gboolean elan_match_enroll (const ElanFrame *frames, guint n_frames, GBytes **tmpl, GError **error);

/* Compares a usable finger frame with a stored template; *matched only if TRUE is returned. */
gboolean elan_match_verify (const ElanFrame *frame, GBytes *tmpl, gboolean *matched, GError **error);
