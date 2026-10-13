/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* Shared by sim-sensor.c, sim-transport.c and sim-run.c. */
#pragma once

#include "sim.h"

extern SimSensor    *sim;               /* the scripted sensor in use */
extern GCancellable *sim_cancellable;   /* what fpi_device_get_cancellable returns */
extern guint8        sim_last[16];      /* last command written */
extern guint         sim_warning_count;

/* Fills the buffer of an IN transfer with what the sensor answers after sim_last. */
gboolean sim_answer (FpiUsbTransfer *transfer, GError **error);

/* Serves the next 00 09 image (whole or in chunks) into an IN transfer on EP 0x82. */
void sim_serve_image (FpiUsbTransfer *transfer);
