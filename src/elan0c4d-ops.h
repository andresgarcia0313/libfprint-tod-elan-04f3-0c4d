/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Building blocks of the protocol, all of them state machines (FpiSsm) over
 * fpi_usb_transfer. Every "op" constructor has the same shape so the retry
 * layer can rebuild it on each attempt; `arg` is NULL unless said otherwise.
 */
#pragma once

#include "elan0c4d.h"

typedef gboolean (*ElanCheck) (FpiDeviceElan0c4d *self, GError **error);

/* One USB exchange: write `out` on EP 0x01, then read at least `in_min` bytes. */
typedef struct
{
  const char   *name;
  const guint8 *out;
  gsize         out_len;
  gsize         in_min;   /* 0: nothing to read */
  gsize         in_max;   /* bytes requested from the endpoint */
  guint8        ep_in;
  guint         timeout_ms;
  gboolean      cancellable;
  ElanCheck     check;    /* validates self->resp, may be NULL */
} ElanCmd;

#define ELAN_WRITE(n, c) { n, c, sizeof c, 0, 0, 0, 0, FALSE, NULL }
#define ELAN_QUERY(n, c, min, chk) \
  { n, c, sizeof c, min, MAX (min, 64), ELAN_EP_INFO, ELAN_QUERY_TIMEOUT_MS, FALSE, chk }

/* Wait for a status code on EP 0x83 (40 3e / 40 3f); result in self->wait_code. */
typedef struct
{
  const char   *name;
  const guint8 *cmd;
  gsize         cmd_len;
  const guint8 *wanted;
  gsize         n_wanted;
  guint         timeout_ms;
} ElanWait;

/* 00 0b: stops what the MCU is doing (never_cancel in the vendor tools, safe to resend). */
extern const ElanCmd elan_cancel_cmd;

typedef FpiSsm *(*ElanOpNew) (FpDevice *dev, gconstpointer arg);
typedef void (*ElanTryDone) (FpiSsm *owner, GError *error);

FpiSsm *elan_cmd_ssm_new (FpDevice *dev, gconstpointer arg);      /* arg: const ElanCmd * (copied) */
FpiSsm *elan_script_ssm_new (FpDevice *dev, const ElanCmd *cmds, guint n);
FpiSsm *elan_wait_ssm_new (FpDevice *dev, gconstpointer arg);     /* arg: const ElanWait * (copied) */
FpiSsm *elan_calibrate_ssm_new (FpDevice *dev, gconstpointer arg);
FpiSsm *elan_init_ssm_new (FpDevice *dev, gconstpointer arg);     /* initialize */
FpiSsm *elan_reinit_ssm_new (FpDevice *dev, gconstpointer arg);   /* initialize + calibrate */
FpiSsm *elan_image_ssm_new (FpDevice *dev, gconstpointer arg);    /* 00 09 -> raw, frame */
FpiSsm *elan_prepare_ssm_new (FpDevice *dev, gconstpointer arg);  /* prepare_capture */
FpiSsm *elan_frame_ssm_new (FpDevice *dev, gconstpointer arg);    /* read_frame */
FpiSsm *elan_arm_ssm_new (FpDevice *dev, gconstpointer arg);      /* SSO, release wait, LED, press wait */
FpiSsm *elan_capture_ssm_new (FpDevice *dev);

/* Time left of the current capture attempt and the error raised when it runs out. */
gint64  elan_capture_left_ms (const FpiDeviceElan0c4d *self);
GError *elan_no_finger_error (const FpiDeviceElan0c4d *self);

/* Run `child` under `owner`; unlike a subssm, `done` always gets the outcome (error owned by it). */
void elan_try (FpiSsm *owner, FpiSsm *child, ElanTryDone done);

/* Child of `parent` that is retried after the remedy of each recoverable error. */
FpiSsm *elan_retry_ssm_new (FpDevice *dev, const char *name, ElanOpNew op, gconstpointer arg,
                            gboolean retry_timeouts);

void elan_log_hex (const char *dir, guint8 ep, const guint8 *buf, gsize len);
void elan_log_error (const char *what, const GError *error);
/* Remedy of a recoverable error: [00 0b | reopen interface] + initialization. */
FpiSsm *elan_recover_ssm_new (FpDevice *dev, ElanRemedy remedy);
