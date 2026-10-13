/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* Private to elan0c4d-wait.c and elan0c4d-wait-read.c. */
#pragma once

#include "elan0c4d-ops.h"

enum { WAIT_SEND, WAIT_READ, WAIT_SCAN, WAIT_ABORT, WAIT_STATES };

typedef struct
{
  ElanWait w;
  gint64   deadline_us;
  GError  *pending;  /* why the wait is being aborted */
} Wait;

GError *elan_wait_timeout_error (const Wait *wait);

/* One read of up to 256 bytes on EP 0x83, cancellable; moves the machine to SCAN or ABORT. */
void elan_wait_read (FpiSsm *ssm, FpDevice *dev, gint64 left_ms);
