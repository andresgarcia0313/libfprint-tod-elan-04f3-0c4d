/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* Enroll in several touches, like a phone (elan0c4d-stages.c, elan0c4d-enroll.c). */
#pragma once

#include "elan0c4d-ops.h"

#define ELAN_ENROLL_STAGES 12
#define ELAN_ENROLL_COVERAGE_MIN 0.8  /* a stage needs more finger than a usable frame (0.6) */
#define ELAN_ENROLL_SIMILAR_MAX 0.9   /* correlation with the previous stage from which it is the same touch */
#define ELAN_LIFT_POLL_MS 150         /* pause between frames while waiting for the finger to go */
#define ELAN_PRINT_FORMAT 1           /* version of the data stored in the print (elan0c4d-print.c) */

/* The touches accepted so far; frame and background as captured, plus their 8-bit image. */
struct _ElanEnrollment
{
  guint   n;
  guint16 pixels[ELAN_ENROLL_STAGES][ELAN_PIXELS];
  guint16 background[ELAN_ENROLL_STAGES][ELAN_PIXELS];
  guint8  normalized[ELAN_ENROLL_STAGES][ELAN_PIXELS];
};

/* Capture, report progress, wait for the lift, again; the stages are left in self->enrollment. */
FpiSsm *elan_stages_ssm_new (FpDevice *dev);

/* Print data (format, width, height, normalized stages, template) and back; parse checks the format. */
GVariant *elan_print_data_new (const ElanEnrollment *enrollment, GBytes *tmpl);
gboolean  elan_print_data_parse (GVariant *data, GPtrArray **frames, GBytes **tmpl, GError **error);
