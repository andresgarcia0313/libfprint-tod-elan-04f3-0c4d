/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Simulated sensor for the state-machine tests. The test executable defines
 * fpi_usb_transfer_submit (and a few more) itself, so the real driver code
 * runs against scripted answers and no USB device is ever touched.
 */
#pragma once

#include "elan0c4d-ops.h"

/* MOVED: the same ridges shifted (another contact); PARTIAL: 11 of 15 block rows (coverage 0.73). */
typedef enum { SIM_EMPTY, SIM_FINGER, SIM_ZEROS, SIM_SATURATED, SIM_MOVED, SIM_PARTIAL } SimImage;

#define SIM_BLOCK 0xFE  /* a 40 3f answer that never comes: only a cancellation ends the read */

typedef struct
{
  guint           ready_after;    /* polls of 40 23 answering 0x00 before 0x03 */
  const SimImage *images;         /* answers to successive 00 09; the last one repeats */
  guint           n_images;
  gsize           image_chunk;    /* 0: whole image per read, else short reads of this size */
  const guint8   *press;          /* answers to successive 40 3f; the last one repeats */
  guint           n_press;
  guint8          fault_cmd[2];   /* the read after this command fails ... */
  guint           fault_count;    /* ... this many times ... */
  GQuark          fault_domain;   /* ... with this error */
  gint            fault_code;
  guint           polls, next_image, next_press;  /* run state */
  GString        *wire;           /* every write as hex, separated by spaces */
} SimSensor;

/* Same sequences as the Python reference, as they appear on the wire. */
#define W_INIT "996001 000b 9a10 402108 402109 4019 402a 460790010400000000"
#define W_CAL "4024 4023"
#define W_SSO "460780010400000000"
#define W_PREPARE "000b " W_SSO " 460790010400000000 9a10 9968 996001 4024 4023 0009 9a14"

/* One fpi_device_enroll_progress call: stages completed and the FpDeviceRetry code, -1 if none. */
typedef struct { gint stages; gint retry; } SimProgress;

extern GArray *sim_progress;    /* SimProgress reported since sim_start */
extern GError *sim_completion;  /* error of fpi_device_enroll_complete */
extern gboolean sim_completed;  /* fpi_device_enroll_complete was called */

FpiDeviceElan0c4d *sim_start (SimSensor *sensor);  /* new headless device wired to `sensor` */
GError *sim_run (FpiSsm *ssm);                     /* run to completion; the error or NULL */
void    sim_cancel_in (guint ms);                  /* cancel the action cancellable after ms */
guint   sim_warnings (void);                       /* warnings logged by the driver so far */
void    sim_end (FpiDeviceElan0c4d *device);
/* Runs the enroll action until it completes (at most 20 s); the error it reported or NULL. */
const GError *sim_run_enroll (FpiDeviceElan0c4d *device);

/* Device whose 00 09 answers are, per touch: background, the touch, an empty frame (finger lifted). */
FpiDeviceElan0c4d *sim_start_touches (SimSensor *sensor, const SimImage *touches, guint n);
/* n progress reports; the one at `retry_at` carries `retry` and repeats the count, the rest count one. */
void sim_assert_progress (guint n, guint retry_at, gint retry);
