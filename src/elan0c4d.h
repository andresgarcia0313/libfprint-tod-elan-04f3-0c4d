/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* FpDevice of the ELAN 04f3:0c4d (rev 0x2410): shared state of all state machines. */
#pragma once

#include "drivers_api.h"
#include "elan0c4d-error.h"
#include "elan0c4d-proto.h"
#include "elan0c4d-quality.h"

#define FPI_TYPE_DEVICE_ELAN0C4D (fpi_device_elan0c4d_get_type ())
G_DECLARE_FINAL_TYPE (FpiDeviceElan0c4d, fpi_device_elan0c4d, FPI, DEVICE_ELAN0C4D, FpDevice)

/* Reserved to one operation at a time: libfprint serializes the actions of a device. */
struct _FpiDeviceElan0c4d
{
  FpDevice parent;

  guint8         resp[ELAN_RESP_MAX];  /* last answer read on EP 0x83 */
  gsize          resp_len;
  guint8         raw[ELAN_RAW_SIZE];   /* last 00 09 image, little endian 16 bit */
  guint16        frame[ELAN_PIXELS];   /* the same image as samples */
  guint16        background[ELAN_PIXELS];
  ElanAssessment assessment;           /* classification of `frame` */
  guint8         wait_code;            /* decisive code of the last wait */
  gint64         deadline_us;          /* end of the current capture attempt */
  gint           capture_timeout_ms;   /* how long a capture waits for a usable finger */
  gboolean       claimed;              /* interface 0 is held by us */
};

/* Action entry points (elan0c4d-actions.c), installed as vfuncs by the class. */
void elan_action_enroll (FpDevice *dev);
void elan_action_verify (FpDevice *dev);
void elan_action_identify (FpDevice *dev);

/* Open: claim, initialize, calibrate. Close: best-effort cancel, release (elan0c4d-lifecycle.c). */
void elan_open (FpDevice *dev);
void elan_close (FpDevice *dev);
