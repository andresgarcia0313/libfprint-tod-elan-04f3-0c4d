/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* Errors of the ELAN 0c4d driver and the remedy each one gets (elan_errors.py). */
#pragma once

#include <glib.h>

typedef enum {
  ELAN_ERROR_PROTOCOL,     /* the sensor answered something not allowed at that point */
  ELAN_ERROR_TIMEOUT,      /* no answer in time; cancel (00 0b) and retry */
  ELAN_ERROR_KEY_LOST,     /* 0xCC / bad session status; re-send 99 60 01 */
  ELAN_ERROR_CALIBRATION,  /* never reached 0x03 */
  ELAN_ERROR_FORBIDDEN,    /* a forbidden command was about to leave the host */
  ELAN_ERROR_EXHAUSTED,    /* all attempts used */
} ElanError;

#define ELAN_ERROR (elan_error_quark ())
GQuark elan_error_quark (void);

typedef enum {
  ELAN_REMEDY_NONE,    /* fatal or cancelled by the user: propagate */
  ELAN_REMEDY_REINIT,  /* 00 0b + initialization */
  ELAN_REMEDY_REKEY,   /* 99 60 01 + initialization */
  ELAN_REMEDY_REOPEN,  /* release and claim the interface, then initialize */
} ElanRemedy;

ElanRemedy elan_error_remedy (const GError *error);
gboolean   elan_error_is_timeout (const GError *error);

/* Takes the error and returns one in FP_DEVICE_ERROR (or the cancellation untouched). */
GError *elan_error_to_device (GError *error);
