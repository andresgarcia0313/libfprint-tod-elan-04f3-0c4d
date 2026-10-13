/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* Synthetic frames for the tests: no biometric data, only patterns with known statistics. */
#pragma once

#include <glib.h>
#include "elan0c4d-proto.h"

void synth_flat (guint16 *px, guint16 value);

/* Quiet sensor: 3050 plus noise of +-40, like the observed resting level. */
void synth_empty (guint16 *px);

/* Crests of period 4 (0, 600, 1200, 600) on the first `block_rows` rows of 10x10 blocks. */
void synth_ridges (guint16 *px, guint block_rows);
