/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * C boundary of the OpenCV comparator (elan0c4d-sift*.cpp). Nothing C++ or
 * OpenCV crosses it: every exception is caught on the C++ side and comes
 * back as a status and a message, so the driver never unwinds through C.
 */
#pragma once

#include <glib.h>

G_BEGIN_DECLS

typedef enum {
  ELAN_SIFT_OK,
  ELAN_SIFT_BAD_TEMPLATE,  /* the stored template is not one this code wrote */
  ELAN_SIFT_NO_FEATURES,   /* the images have nothing to match on */
  ELAN_SIFT_FAILED,        /* OpenCV or the allocator failed */
} ElanSiftStatus;

/* images: n 8-bit width x height views (elan_quality_normalize) of one finger. */
ElanSiftStatus elan_sift_enroll (const guint8 *const *images, guint n, guint width, guint height,
                                 GBytes **tmpl, gchar **message);

/* *best: largest number of geometrically consistent keypoint matches with any view. */
ElanSiftStatus elan_sift_score (const guint8 *image, guint width, guint height, GBytes *tmpl,
                                gint *best, gchar **message);

G_END_DECLS
