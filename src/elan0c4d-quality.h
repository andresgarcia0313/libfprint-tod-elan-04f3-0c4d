/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* Frame classifier (EMPTY / FINGER / INVALID); port of elan_quality.py. */
#pragma once

#include <glib.h>

typedef enum {
  ELAN_FRAME_EMPTY,
  ELAN_FRAME_FINGER,
  ELAN_FRAME_INVALID,
} ElanFrameKind;

typedef struct
{
  ElanFrameKind kind;
  gdouble       mean;      /* mean raw value of the whole frame */
  gdouble       coverage;  /* share of 10x10 blocks that look like skin */
  gdouble       ridge;     /* mean block energy (mean |gradient|) */
} ElanAssessment;

/* px: width * height raw 14-bit samples, row after row. */
ElanAssessment elan_quality_assess (const guint16 *px, guint width, guint height);

const char *elan_frame_kind_name (ElanFrameKind kind);
