/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* Shared by test-match.c and test-match-data.c. */
#pragma once

#include <glib.h>
#include "elan0c4d-proto.h"

extern guint16 shots[4][ELAN_PIXELS];

/* Template from four shifted windows of synthetic surface `seed`. */
GBytes *enroll_surface (guint32 seed);
